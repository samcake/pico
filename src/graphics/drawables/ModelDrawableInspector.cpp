// ModelDrawableInspector.cpp
//
// Sam Gateau - June 2020
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
#include "ModelDrawableInspector.h"

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

#include "ModelInspectorPart_vert.h"
#include "ModelInspectorPart_frag.h"

//using namespace view3d;
namespace graphics
{

    ModelDrawableInspectorFactory::ModelDrawableInspectorFactory() :
        _sharedUniforms(std::make_shared<ModelDrawableInspectorUniforms>()) {

    }
    ModelDrawableInspectorFactory::~ModelDrawableInspectorFactory() {

    }

    // Custom data uniforms
    struct ModelObjectData {
        uint32_t nodeID{0};
        uint32_t partID{0};
        uint32_t numNodes{ 0 };
        uint32_t numParts{ 0 };
        uint32_t numMaterials{ 0 };
    };

    void ModelDrawableInspectorFactory::allocateGPUShared(const graphics::DevicePointer& device) {

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
        auto shader_vertex_src = ModelInspectorPart_vert::getSource();
        auto shader_pixel_src = ModelInspectorPart_frag::getSource();

        // test: create shader
        graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "main", "", shader_vertex_src, ModelInspectorPart_vert::getSourceFilename() };
        graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main", "", shader_pixel_src, ModelInspectorPart_frag::getSourceFilename() };
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




    graphics::ModelDrawableInspector* ModelDrawableInspectorFactory::createModel(const graphics::DevicePointer& device, const document::ModelPointer& model, const ModelDrawable* srcDrawable) {

        auto modelDrawable = new graphics::ModelDrawableInspector();

        modelDrawable->_inspectedModelDrawable = srcDrawable;

        // Define the local nodes used by the model with the original transforms and the parents
        modelDrawable->_localNodeTransforms = srcDrawable->_localNodeTransforms;
        modelDrawable->_localNodeParents = srcDrawable->_localNodeParents;
        // Define the items
        modelDrawable->_localItems = srcDrawable->_localItems;
        // Define the shapes
        modelDrawable->_shapes = srcDrawable->_shapes;
        // Define the cameras
        modelDrawable->_localCameras = srcDrawable->_localCameras;


        modelDrawable->_uniforms = _sharedUniforms;
        modelDrawable->_indexBuffer = srcDrawable->_indexBuffer;
        modelDrawable->_vertexBuffer = srcDrawable->_vertexBuffer;
        modelDrawable->_vertexAttribBuffer = srcDrawable->_vertexAttribBuffer;
        modelDrawable->_partBuffer = srcDrawable->_partBuffer;
        modelDrawable->_shapes = srcDrawable->_shapes;

        // Also need aversion of the parts and their bound on the cpu side
        modelDrawable->_parts = srcDrawable->_parts;
        modelDrawable->_partAABBs = srcDrawable->_partAABBs;

        // Materials
        std::vector<ModelMaterial> materials;
        for (const auto& m : model->_materials) {
            ModelMaterial mm;
            mm.color = m._baseColor;
            mm.metallic = m._metallicFactor;
            mm.roughness = m._roughnessFactor;
            mm.baseColorTexture = m._baseColorTexture;
            mm.normalTexture = m._normalTexture;
            mm.rmaoTexture = m._roughnessMetallicTexture;
            mm.emissiveTexture = m._emissiveTexture;
            materials.emplace_back(mm);
        }

        // material buffer
        modelDrawable->_materialBuffer = srcDrawable->_materialBuffer;
        modelDrawable->_albedoTexture = srcDrawable->_albedoTexture;
        
        // Model local bound is the containing box for all the local items of the model
        modelDrawable->_bound = srcDrawable->_bound;

        return modelDrawable;
    }

   void ModelDrawableInspectorFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        const graphics::ScenePointer& scene,
        const graphics::CameraPointer& camera,
        graphics::ModelDrawableInspector& model)
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
                    batch->uploadTextureFromInitdata(device, albedoTex);
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
           auto part = new ModelDrawableInspectorPart();
           part->_bound = model._partAABBs[d];

            auto partNumIndices = model._parts[d].numIndices;
           // And now a render callback where we describe the rendering sequence
           graphics::DrawObjectCallback drawCallback = [d, partNumIndices, numNodes, numParts, numMaterials, descriptorSet, pipeline](
               const NodeID node,
               const graphics::CameraPointer& camera,
               const graphics::SwapchainPointer& swapchain,
               const graphics::DevicePointer& device,
               const graphics::BatchPointer& batch) {

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

   graphics::ItemIDs ModelDrawableInspectorFactory::createModelParts(
                    const graphics::NodeID root,
                    const graphics::ScenePointer& scene,
                    graphics::ModelDrawableInspector& model) {
   
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