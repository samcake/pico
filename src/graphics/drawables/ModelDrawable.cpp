// ModelDrawable.cpp
//
// Sam Gateau - January 2020
// 
// MIT License
//
// Copyright (c) 2020 Sam Gateau
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#include "ModelDrawable.h"

#include "gpu/Device.h"
#include "gpu/Batch.h"
#include "gpu/Shader.h"
#include "gpu/Resource.h"
#include "gpu/Pipeline.h"
#include "gpu/Descriptor.h"
#include "gpu/Swapchain.h"

#include "render/Renderer.h"
#include "render/Camera.h"
#include "render/Scene.h"
#include "render/Drawable.h"
#include "render/Viewport.h"
#include "render/Mesh.h"

#include "ModelPart_vert.h"
#include "ModelPart_frag.h"

//using namespace view3d;
namespace graphics
{

    ModelDrawableFactory::ModelDrawableFactory() :
        _sharedUniforms(std::make_shared<ModelDrawableUniforms>()) {

    }
    ModelDrawableFactory::~ModelDrawableFactory() {

    }

    // Custom data uniforms
    struct ModelObjectData {
        uint32_t nodeID{0};
        uint32_t partID{0};
        uint32_t numNodes{ 0 };
        uint32_t numParts{ 0 };
        uint32_t numMaterials{ 0 };
    };

    void ModelDrawableFactory::allocateGPUShared(const graphics::DevicePointer& device) {

        // Let's describe the pipeline Descriptors layout
        graphics::DescriptorLayouts descriptorLayouts{
            { graphics::DescriptorType::UNIFORM_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::ALL_GRAPHICS, 1, sizeof(ModelObjectData) >> 2},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 0, 1}, // Node Transform
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::ALL_GRAPHICS, 1, 1}, // Part
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 2, 1}, // Index
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 3, 1}, // Vertex
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 4, 1}, // Attrib
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::PIXEL, 5, 1},  // Material
            { graphics::DescriptorType::RESOURCE_TEXTURE, graphics::ShaderStage::PIXEL, 0, 1},  // Albedo Texture
            { graphics::DescriptorType::SAMPLER, graphics::ShaderStage::PIXEL, 0, 1},
        };

        graphics::DescriptorSetLayoutInit descriptorSetLayoutInit{ descriptorLayouts };
        auto descriptorSetLayout = device->createDescriptorSetLayout(descriptorSetLayoutInit);

        // And a Pipeline

        // Load shaders (as stored in the resources)
        auto shader_vertex_src = ModelPart_vert::getSource();
        auto shader_pixel_src = ModelPart_frag::getSource();

        // test: create shader
        graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "main", "", shader_vertex_src, ModelPart_vert::getSourceFilename() };
        graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main", "", shader_pixel_src, ModelPart_frag::getSourceFilename() };
        graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

        graphics::ProgramInit programInit{ vertexShader, pixelShader };
        graphics::ShaderPointer programShader = device->createProgram(programInit);

        graphics::GraphicsPipelineStateInit pipelineInit{
                    programShader,
                    StreamLayout(),
                    graphics::PrimitiveTopology::TRIANGLE,
                    descriptorSetLayout,
                    RasterizerState().withCullBack(),
                    true, // enable depth
                    BlendState()
        };
        _pipeline = device->createGraphicsPipelineState(pipelineInit);
    }




    graphics::ModelDrawable* ModelDrawableFactory::createModel(const graphics::DevicePointer& device, const document::ModelPointer& model) {

        auto modelDrawable = new graphics::ModelDrawable();

        // Define the local nodes used by the model with the original transforms and the parents
        modelDrawable->_localNodeTransforms.reserve(model->_nodes.size());
        modelDrawable->_localNodeParents.reserve(model->_nodes.size());
        for (const auto& n : model->_nodes) {
            modelDrawable->_localNodeTransforms.emplace_back(n._transform);
            modelDrawable->_localNodeParents.emplace_back(n._parent);
        }

        // Define the items
        modelDrawable->_localItems.reserve(model->_items.size());
        for (const auto& si : model->_items) {
            modelDrawable->_localItems.emplace_back(ModelItem{si._node, si._mesh, si._camera });
        }

        // Define the shapes
        modelDrawable->_shapes.reserve(model->_meshes.size());
        for (const auto& m : model->_meshes) {
            modelDrawable->_shapes.emplace_back(ModelShape{ m._primitiveStart, m._primitiveCount });
        }

        // Define the cameras
        modelDrawable->_localCameras.reserve(model->_cameras.size());
        for (const auto& cam : model->_cameras) {
            modelDrawable->_localCameras.emplace_back(ModelCamera{ cam._projection });
        }

        // Build the geometry vb, ib and pb
        // as long as the vertex buffer is  less than 65535 the indices can be uint16
        std::vector<core::vec4> vertex_buffer;
        std::vector<core::vec4> vertex_attrib_buffer;
        std::vector<uint16_t> index_buffer;
        std::vector<ModelPart> parts;
        std::vector<core::aabox3> partAABBs;
        core::aabox3 bound;

        bool first = true;
        for (const auto& p : model->_primitives) {

            const auto& indexAccess = model->_accessors[p._indices];
            const auto& indexView = model->_bufferViews[indexAccess._bufferView];
            const auto& indexBuffer = model->_buffers[indexView._buffer];

            const auto& posAccess = model->_accessors[p._positions];
            const auto& posView = model->_bufferViews[posAccess._bufferView];
            const auto& posBuffer = model->_buffers[posView._buffer];

            uint32_t attribsOffset = -1;
            if (p._texcoords != document::model::INVALID_INDEX) {
                attribsOffset = (uint32_t) vertex_attrib_buffer.size();
            }   
            ModelPart part{ indexAccess._elementCount, (uint32_t) index_buffer.size(), (uint32_t) vertex_buffer.size(), attribsOffset, p._material};
            parts.emplace_back(part);

            auto indexStride = (indexView._byteStride ? indexView._byteStride : document::model::componentTypeSize(indexAccess._componentType));
            uint32_t indexMask = document::model::componentTypeInt32Mask(indexAccess._componentType);
            for (uint32_t i = 0; i < indexAccess._elementCount; ++i) {
                auto index = (*(uint32_t*) (indexBuffer._bytes.data() + indexView._byteOffset + indexAccess._byteOffset + indexStride * i)) & indexMask;
                index_buffer.emplace_back( index );
            }

            auto posStride = (posView._byteStride ? posView._byteStride : document::model::elementTypeComponentCount(posAccess._elementType) *sizeof(float));
            for (uint32_t i = 0; i < posAccess._elementCount; ++i) {
                auto pos = (float*)(posBuffer._bytes.data() + posView._byteOffset + posAccess._byteOffset + posStride * i);
                vertex_buffer.emplace_back( *pos, *(pos+1), *(pos + 2), *(pos + 3));
            }

            partAABBs.emplace_back(posAccess._aabb);
            if (first) {
                bound = posAccess._aabb;
                first = false;
            }
            else {
                bound = core::aabox3::fromBound(bound, posAccess._aabb);
            }
            
            if (p._texcoords != document::model::INVALID_INDEX) {
                const auto& texcoordAccess = model->_accessors[p._texcoords];
                const auto& texcoordView = model->_bufferViews[texcoordAccess._bufferView];
                const auto& texcoordBuffer = model->_buffers[texcoordView._buffer];

                auto texcoordStride = (texcoordView._byteStride ? texcoordView._byteStride : document::model::elementTypeComponentCount(texcoordAccess._elementType) * sizeof(float));
                for (uint32_t i = 0; i < texcoordAccess._elementCount; ++i) {
                    auto texcoord = (float*)(texcoordBuffer._bytes.data() + texcoordView._byteOffset + texcoordAccess._byteOffset + texcoordStride * i);
                    vertex_attrib_buffer.emplace_back(*texcoord, *(texcoord + 1), 0.0f, 0.0f);
                }
            }
        }

        // parts
        BufferInit partBufferInit;
        partBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        partBufferInit.bufferSize = parts.size() * sizeof(ModelPart);
        partBufferInit.hostVisible = true; // TODO Change this to immutable and initialized value
        partBufferInit.firstElement = 0;
        partBufferInit.numElements = parts.size();
        partBufferInit.structStride = sizeof(ModelPart);

        auto pbuniformBuffer = device->createBuffer(partBufferInit);
        memcpy(pbuniformBuffer->_cpuMappedAddress, parts.data(), partBufferInit.bufferSize);

        // vertex buffer
        BufferInit vertexBufferInit;
        vertexBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        vertexBufferInit.bufferSize = vertex_buffer.size() * sizeof(core::vec4);
        vertexBufferInit.hostVisible = true; // TODO Change this to immutable and initialized value
        vertexBufferInit.firstElement = 0;
        vertexBufferInit.numElements = vertex_buffer.size();
        vertexBufferInit.structStride = sizeof(core::vec4);

        auto vbresourceBuffer = device->createBuffer(vertexBufferInit);
        memcpy(vbresourceBuffer->_cpuMappedAddress, vertex_buffer.data(), vertexBufferInit.bufferSize);

        // vertex attrib buffer
        BufferInit vertexattribBufferInit;
        BufferPointer vabresourceBuffer;
        if (vertex_attrib_buffer.size()) {
            vertexattribBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
            vertexattribBufferInit.bufferSize = vertex_attrib_buffer.size() * sizeof(core::vec4);
            vertexattribBufferInit.hostVisible = true; // TODO Change this to immutable and initialized value
            vertexattribBufferInit.firstElement = 0;
            vertexattribBufferInit.numElements = vertex_attrib_buffer.size();
            vertexattribBufferInit.structStride = sizeof(core::vec4);

            vabresourceBuffer = device->createBuffer(vertexattribBufferInit);
            memcpy(vabresourceBuffer->_cpuMappedAddress, vertex_attrib_buffer.data(), vertexattribBufferInit.bufferSize);
        }

        // index buffer
        BufferInit indexBufferInit;
        indexBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        indexBufferInit.bufferSize = index_buffer.size() * sizeof(uint16_t);
        indexBufferInit.hostVisible = true; // TODO Change this to immutable and initialized value
        indexBufferInit.firstElement = 0;
        indexBufferInit.numElements = index_buffer.size();
        indexBufferInit.structStride = sizeof(uint16_t);

        auto ibresourceBuffer = device->createBuffer(indexBufferInit);
        memcpy(ibresourceBuffer->_cpuMappedAddress, index_buffer.data(), indexBufferInit.bufferSize);

        modelDrawable->_uniforms = _sharedUniforms;
        modelDrawable->_indexBuffer = ibresourceBuffer;
        modelDrawable->_vertexBuffer = vbresourceBuffer;
        modelDrawable->_vertexAttribBuffer = vabresourceBuffer;
        modelDrawable->_partBuffer = pbuniformBuffer;

        // Also need aversion of the parts and their bound on the cpu side
        modelDrawable->_parts = std::move(parts);
        modelDrawable->_partAABBs = std::move(partAABBs);

        // Materials
        std::vector<ModelMaterial> materials;
        for (const auto& m : model->_materials) {
            ModelMaterial mm;
            mm.color = m._baseColor;
            mm.metallic = m._metallicFactor;
            mm.roughness = m._roughnessFactor;
            mm.baseColorTexture = m._baseColorTexture;
            materials.emplace_back(mm);
        }

        // material buffer
        BufferInit materialBufferInit;
        materialBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        materialBufferInit.bufferSize = materials.size() * sizeof(ModelMaterial);
        materialBufferInit.hostVisible = true; // TODO Change this to immutable and initialized value
        materialBufferInit.firstElement = 0;
        materialBufferInit.numElements = materials.size();
        materialBufferInit.structStride = sizeof(ModelMaterial);

        auto mbresourceBuffer = device->createBuffer(materialBufferInit);
        memcpy(mbresourceBuffer->_cpuMappedAddress, materials.data(), materialBufferInit.bufferSize);

        modelDrawable->_materialBuffer = mbresourceBuffer;

        // Allocate the textures
        if (model->_images.size()) {
            auto& image0 = model->_images[2];
            TextureInit albedoTexInit;
            albedoTexInit.width = image0._desc.width;
            albedoTexInit.height = image0._desc.height;
            albedoTexInit.initData = image0._pixels;
            auto albedoresourceTexture = device->createTexture(albedoTexInit);

            modelDrawable->_albedoTexture = albedoresourceTexture;
        }
        
        // Model local bound is the containing box for all the local items of the model
        core::aabox3 model_aabb;
        for (const auto& i : modelDrawable->_localItems) {
            if (i.shape != MODEL_INVALID_INDEX) {
                auto nodeIdx = i.node;
                core::mat4x3 transform = modelDrawable->_localNodeTransforms[nodeIdx];
                nodeIdx = modelDrawable->_localNodeParents[nodeIdx];
                while(nodeIdx != INVALID_NODE_ID) {
                    transform = core::mul(modelDrawable->_localNodeTransforms[nodeIdx], transform);
                    nodeIdx = modelDrawable->_localNodeParents[nodeIdx];
                }

                const auto& s = modelDrawable->_shapes[i.shape];
                core::aabox3 shape_aabb = modelDrawable->_partAABBs[s.partOffset];
                for (int p = 1; p < s.numParts; ++p) {
                    shape_aabb = core::aabox3::fromBound(shape_aabb, modelDrawable->_partAABBs[p + s.partOffset]);
                };
                shape_aabb = core::aabox_transformFrom(transform, shape_aabb);
                model_aabb = core::aabox3::fromBound(model_aabb, shape_aabb);
            }
        }
        modelDrawable->_bound = model_aabb;

        return modelDrawable;
    }

   void ModelDrawableFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        const graphics::ScenePointer& scene,
        const graphics::CameraPointer& camera,
        graphics::ModelDrawable& model)
    {
       // It s time to create a descriptorSet that matches the expected pipeline descriptor set
        // then we will assign a uniform buffer in it
       graphics::DescriptorSetInit descriptorSetInit{
           _pipeline->getDescriptorSetLayout()
       };
       auto descriptorSet = device->createDescriptorSet(descriptorSetInit);
       model._descriptorSet = descriptorSet;

       // Assign the Camera UBO just created as the resource of the descriptorSet
       // auto descriptorObjects = descriptorSet->buildDescriptorObjects();
       graphics::DescriptorObject camera_uboDescriptorObject;
       camera_uboDescriptorObject._uniformBuffers.push_back(camera->getGPUBuffer());
       graphics::DescriptorObject transform_rboDescriptorObject;
       transform_rboDescriptorObject._buffers.push_back(scene->_nodes._transforms_buffer);

       graphics::DescriptorObject pb_rboDescriptorObject;
       pb_rboDescriptorObject._buffers.push_back(model.getPartBuffer());
       graphics::DescriptorObject ib_rboDescriptorObject;
       ib_rboDescriptorObject._buffers.push_back(model.getIndexBuffer());
       graphics::DescriptorObject vb_rboDescriptorObject;
       vb_rboDescriptorObject._buffers.push_back(model.getVertexBuffer());
       graphics::DescriptorObject ab_rboDescriptorObject;
       ab_rboDescriptorObject._buffers.push_back(model.getVertexAttribBuffer());

       graphics::DescriptorObject mb_rboDescriptorObject;
       mb_rboDescriptorObject._buffers.push_back(model.getMaterialBuffer());
       graphics::DescriptorObject texDescriptorObject;
       texDescriptorObject._textures.push_back(model.getAlbedoTexture());
       graphics::DescriptorObject samplerDescriptorObject;
       graphics::SamplerInit samplerInit{};
       auto sampler = device->createSampler(samplerInit);
       samplerDescriptorObject._samplers.push_back(sampler);

       graphics::DescriptorObjects descriptorObjects = {
            camera_uboDescriptorObject,
            transform_rboDescriptorObject, 
            pb_rboDescriptorObject,
            ib_rboDescriptorObject,
            vb_rboDescriptorObject,
            ab_rboDescriptorObject,
            mb_rboDescriptorObject,
            texDescriptorObject,
            samplerDescriptorObject
       };
       device->updateDescriptorSet(descriptorSet, descriptorObjects);


       auto numVertices = model.getVertexBuffer()->getNumElements();
       auto numIndices = model.getIndexBuffer()->getNumElements();
       auto vertexStride = model.getVertexBuffer()->_init.structStride;
       auto numParts = model.getPartBuffer()->getNumElements();
       auto numMaterials = model.getMaterialBuffer()->getNumElements();

       // NUmber of nodes in the model
       auto numNodes = model._localNodeTransforms.size();

       auto pipeline = this->_pipeline;
       auto albedoTex = model.getAlbedoTexture();

       // And now a render callback where we describe the rendering sequence
       graphics::DrawObjectCallback drawCallback = [descriptorSet, pipeline, albedoTex](
           const NodeID node,
           const graphics::CameraPointer& camera,
           const graphics::SwapchainPointer& swapchain,
           const graphics::DevicePointer& device,
           const graphics::BatchPointer& batch) {
            
            static bool first{ true };
            if (first) {
                first = false;
                if (albedoTex) {
                    batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::SHADER_RESOURCE, graphics::ResourceState::COPY_DEST, albedoTex);
                    batch->uploadInitTexture(device, albedoTex);
                    batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::COPY_DEST, graphics::ResourceState::SHADER_RESOURCE, albedoTex);
                }
            }

            batch->bindPipeline(pipeline);
            batch->setViewport(camera->getViewportRect());
            batch->setScissor(camera->getViewportRect());

            batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet);
       };
       model._drawcall = drawCallback;
       model._drawableID = scene->createDrawable(model).id();

       // one drawable per part
       DrawableIDs drawables;
       for (int d = 0; d < model._partAABBs.size(); ++d) {
           auto part = new ModelDrawablePart();
           part->_bound = model._partAABBs[d];

            auto partNumIndices = model._parts[d].numIndices;
           // And now a render callback where we describe the rendering sequence
           graphics::DrawObjectCallback drawCallback = [d, partNumIndices, numNodes, numParts, numMaterials, descriptorSet, pipeline](
               const NodeID node,
               const graphics::CameraPointer& camera,
               const graphics::SwapchainPointer& swapchain,
               const graphics::DevicePointer& device,
               const graphics::BatchPointer& batch) {
               /*    batch->bindPipeline(pipeline);
                   batch->setViewport(camera->getViewportRect());
                   batch->setScissor(camera->getViewportRect());

                   batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet);
*/
                   ModelObjectData odata{ (int32_t)node, (int32_t)d, numNodes, numParts, numMaterials };
                   batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 1, sizeof(ModelObjectData), (const uint8_t*)&odata);
                   batch->draw(partNumIndices, 0);
           };

           part->_drawcall = drawCallback;

           auto partDrawable = scene->createDrawable(*part);
           drawables.emplace_back(partDrawable.id());
       }

       model._partDrawables = drawables;

    }

   graphics::ItemIDs ModelDrawableFactory::createModelParts(
                    const graphics::NodeID root,
                    const graphics::ScenePointer& scene,
                    graphics::ModelDrawable& model) {
   
        auto rootNode = scene->createNode(core::mat4x3(), root);

        
        // Allocating the new instances of scene::nodes, one per local node
        auto modelNodes = scene->createNodeBranch(rootNode.id(), model._localNodeTransforms, model._localNodeParents);

        // Allocate the new scene::items combining the localItem's node with every shape parts
        graphics::ItemIDs items;
        
        // first item is the model drawable
        items.emplace_back(scene->createItem(rootNode.id(), model._drawableID).id());

        for (const auto& li : model._localItems) {
            if (li.shape != MODEL_INVALID_INDEX) {
                const auto& s = model._shapes[li.shape];
                for (uint32_t si = 0; si < s.numParts; ++si) {
                    items.emplace_back(scene->createItem(modelNodes[li.node], model._partDrawables[si + s.partOffset]).id());
                }
            }
            if (li.camera != MODEL_INVALID_INDEX) {
                
            }
        }

        return items; 
   }


} // !namespace graphics