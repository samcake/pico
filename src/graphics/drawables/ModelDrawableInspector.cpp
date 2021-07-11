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
#include "gpu/Framebuffer.h"

#include "render/Renderer.h"
#include "render/Camera.h"
#include "render/Scene.h"
#include "render/Drawable.h"
#include "render/Viewport.h"
#include "render/Mesh.h"

#include "ModelInspectorPart_vert.h"
#include "ModelInspectorPart_frag.h"

#include <functional>
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
        uint32_t numEdges{ 0 };
        uint32_t drawMode{ 0 };
    };

    
    uint32_t ModelDrawableInspectorUniforms::buildFlags() const {
        return (uint32_t) 
                  (renderUVSpace) * RENDER_UV_SPACE_BIT 
                | (showUVGrid) * SHOW_UV_GRID_BIT;
    }

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
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::ALL_GRAPHICS, 5, 1}, // Edge

            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::PIXEL, 6, 1},  // Material
            { graphics::DescriptorType::RESOURCE_TEXTURE, graphics::ShaderStage::PIXEL, 0, 1},  // Albedo Texture
            { graphics::DescriptorType::SAMPLER, graphics::ShaderStage::PIXEL, 0, 1},
        };

        graphics::DescriptorSetLayoutInit descriptorSetLayoutInit{ descriptorLayouts };
        auto descriptorSetLayout = device->createDescriptorSetLayout(descriptorSetLayoutInit);

        // Let's describe the pipeline Descriptors layout
        graphics::DescriptorLayouts edge_descriptorLayouts{
            { graphics::DescriptorType::UNIFORM_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::ALL_GRAPHICS, 1, sizeof(ModelObjectData) >> 2},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 0, 1}, // Node Transform
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::ALL_GRAPHICS, 1, 1}, // Part
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 2, 1}, // Index
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 3, 1}, // Vertex
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 4, 1}, // Attrib
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::ALL_GRAPHICS, 5, 1}, // Edge

        };

        graphics::DescriptorSetLayoutInit edge_descriptorSetLayoutInit{ edge_descriptorLayouts };
        auto edge_descriptorSetLayout = device->createDescriptorSetLayout(edge_descriptorSetLayoutInit);


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

        graphics::ShaderInit whitepixelShaderInit{ graphics::ShaderType::PIXEL, "white", "", shader_pixel_src, ModelInspectorPart_frag::getSourceFilename() };
        graphics::ShaderPointer whitepixelShader = device->createShader(whitepixelShaderInit);

        graphics::ProgramInit edge_programInit{ vertexShader, whitepixelShader };
        graphics::ShaderPointer edge_programShader = device->createProgram(edge_programInit);

        // Default pipeline draw faces
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

        // Inspect UVMap pipeline draw edges
        graphics::GraphicsPipelineStateInit pipelineInitUVMap{
                    programShader,
                    StreamLayout(),
                    graphics::PrimitiveTopology::LINE,
                    descriptorSetLayout,
                    RasterizerState().withAntialiasedLine().withConservativeRasterizer(),
                    true, // enable depth
                    BlendState()
        };
        _pipelineInspectUVMap = device->createGraphicsPipelineState(pipelineInitUVMap);

        // Make UVMap pipeline draw edges
        graphics::GraphicsPipelineStateInit pipelineInitMakeUVMap{
                    edge_programShader,
                    StreamLayout(),
                    graphics::PrimitiveTopology::LINE,
                    edge_descriptorSetLayout,
                    RasterizerState().withConservativeRasterizer(),
                    false,
                    BlendState()
        };
        _pipelineMakeSeamMap = device->createGraphicsPipelineState(pipelineInitMakeUVMap);
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
        modelDrawable->_vertices = srcDrawable->_vertices;
        modelDrawable->_vertex_attribs = srcDrawable->_vertex_attribs;
        modelDrawable->_indices = srcDrawable->_indices;
        modelDrawable->_parts = srcDrawable->_parts;
        modelDrawable->_partAABBs = srcDrawable->_partAABBs;

        // material buffer
        modelDrawable->_materialBuffer = srcDrawable->_materialBuffer;
        modelDrawable->_albedoTexture = srcDrawable->_albedoTexture;
        
        // Model local bound is the containing box for all the local items of the model
        modelDrawable->_bound = srcDrawable->_bound;

        // 
        auto edges = computeUVSeamsEdges(*modelDrawable);

        // seam edge buffer
        BufferInit edgeBufferInit;
        edgeBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        edgeBufferInit.bufferSize = edges.size() * sizeof(ModelEdge);
        edgeBufferInit.hostVisible = true; // TODO Change this to immutable and initialized value
        edgeBufferInit.firstElement = 0;
        edgeBufferInit.numElements = edges.size();
        edgeBufferInit.structStride = sizeof(ModelEdge);

        auto ebuniformBuffer = device->createBuffer(edgeBufferInit);
        memcpy(ebuniformBuffer->_cpuMappedAddress, edges.data(), edgeBufferInit.bufferSize);

        modelDrawable->_edges = std::move(edges);
        modelDrawable->_edgeBuffer = ebuniformBuffer;

        // Making one "seam map" which will be rt once with the seam edges and then used to fetch from
        TextureInit edgeMapInit;
        edgeMapInit.width = modelDrawable->_albedoTexture->_init.width;
        edgeMapInit.height = modelDrawable->_albedoTexture->_init.height;
        edgeMapInit.usage = ResourceUsage::RENDER_TARGET;
        auto edgeMap = device->createTexture(edgeMapInit);
        modelDrawable->_edgeTexture = edgeMap;

        // And the framebuffer to render into it
        FramebufferInit edgeFramebufferInit;
        edgeFramebufferInit.colorTargets.push_back(edgeMap);
        auto edgeFramebuffer = device->createFramebuffer(edgeFramebufferInit);
        modelDrawable->_edgeFramebuffer = edgeFramebuffer;

        return modelDrawable;
    }

    void ModelDrawableInspectorFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        const graphics::ScenePointer& scene,
        const graphics::CameraPointer& camera,
        graphics::ModelDrawableInspector& model)
    {
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

            graphics::DescriptorObject eb_rboDescriptorObject;
            eb_rboDescriptorObject._buffers.push_back(model.getEdgeBuffer());

            graphics::DescriptorObject mb_rboDescriptorObject;
            mb_rboDescriptorObject._buffers.push_back(model.getMaterialBuffer());
            graphics::DescriptorObject texDescriptorObject;
         //   texDescriptorObject._textures.push_back(model.getAlbedoTexture());
            texDescriptorObject._textures.push_back(model._edgeTexture);
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
                eb_rboDescriptorObject,

                mb_rboDescriptorObject,
                texDescriptorObject,
                samplerDescriptorObject
            };
            device->updateDescriptorSet(descriptorSet, descriptorObjects);
        }

        {
            // It s time to create a descriptorSet that matches the expected pipeline descriptor set
            // then we will assign a uniform buffer in it
            graphics::DescriptorSetInit descriptorSetInit{
                _pipelineMakeSeamMap->getDescriptorSetLayout()
            };
            auto descriptorSet = device->createDescriptorSet(descriptorSetInit);
            model._descriptorSetMakeSeamMap = descriptorSet;

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

            graphics::DescriptorObject eb_rboDescriptorObject;
            eb_rboDescriptorObject._buffers.push_back(model.getEdgeBuffer());

       /*     graphics::DescriptorObject mb_rboDescriptorObject;
            mb_rboDescriptorObject._buffers.push_back(model.getMaterialBuffer());
            graphics::DescriptorObject texDescriptorObject;
            texDescriptorObject._textures.push_back(model.getAlbedoTexture());
            graphics::DescriptorObject samplerDescriptorObject;
            graphics::SamplerInit samplerInit{};
            auto sampler = device->createSampler(samplerInit);
            samplerDescriptorObject._samplers.push_back(sampler);
            */
            graphics::DescriptorObjects descriptorObjects = {
                camera_uboDescriptorObject,
                transform_rboDescriptorObject,
                pb_rboDescriptorObject,
                ib_rboDescriptorObject,
                vb_rboDescriptorObject,
                ab_rboDescriptorObject,
                eb_rboDescriptorObject,

          //      mb_rboDescriptorObject,
          //      texDescriptorObject,
          //      samplerDescriptorObject
            };
            device->updateDescriptorSet(descriptorSet, descriptorObjects);
        }

        auto numVertices = model.getVertexBuffer()->getNumElements();
        auto numIndices = model.getIndexBuffer()->getNumElements();
        auto vertexStride = model.getVertexBuffer()->_init.structStride;
        auto numParts = model.getPartBuffer()->getNumElements();
        auto numMaterials = model.getMaterialBuffer()->getNumElements();

        auto numEdges = model.getEdgeBuffer()->getNumElements();

        // NUmber of nodes in the model
        auto numNodes = model._localNodeTransforms.size();

        auto pipeline = this->_pipeline;
        auto albedoTex = model.getAlbedoTexture();
        auto pmodel = &model;
        auto descriptorSet = model._descriptorSet;
        auto makeSeamDescriptorSet = model._descriptorSetMakeSeamMap;
        auto edgeMap = model._edgeTexture;
        auto edgeFramebuffer = model._edgeFramebuffer;

        // And now a render callback where we describe the rendering sequence
        {
            graphics::DrawObjectCallback drawCallback = [descriptorSet, pipeline, albedoTex](
                const NodeID node,
                const graphics::CameraPointer& camera,
                const graphics::SwapchainPointer& swapchain,
                const graphics::DevicePointer& device,
                const graphics::BatchPointer& batch) {
            
             /*   static bool first{ true };
                if (first) {
                    first = false;
                    if (albedoTex) {
                        batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::SHADER_RESOURCE, graphics::ResourceState::COPY_DEST, albedoTex);
                        batch->uploadTextureFromInitdata(device, albedoTex);
                        batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::COPY_DEST, graphics::ResourceState::SHADER_RESOURCE, albedoTex);
                    }
                }*/

                batch->bindPipeline(pipeline);
                batch->setViewport(camera->getViewportRect());
                batch->setScissor(camera->getViewportRect());

                batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet);
            };
            model._drawcall = drawCallback;
            model._drawableID = scene->createDrawable(model).id();
        }

        // one drawable per part
        {
            DrawableIDs drawables;
            for (int d = 0; d < model._partAABBs.size(); ++d) {
                auto part = new ModelDrawableInspectorPart();
                part->_bound = model._partAABBs[d];

                auto partNumIndices = model._parts[d].numIndices;
                // And now a render callback where we describe the rendering sequence
                graphics::DrawObjectCallback drawCallback = [pmodel, d, partNumIndices, numEdges, numNodes, numParts, numMaterials, descriptorSet, pipeline](
                    const NodeID node,
                    const graphics::CameraPointer& camera,
                    const graphics::SwapchainPointer& swapchain,
                    const graphics::DevicePointer& device,
                    const graphics::BatchPointer& batch) {

                        auto flags = pmodel->getUniforms()->buildFlags();
                        ModelObjectData odata{ (int32_t)node, (int32_t)d, numNodes, numParts, numMaterials, numEdges, flags };
                        batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 1, sizeof(ModelObjectData), (const uint8_t*)&odata);
                        batch->draw(partNumIndices, 0);
                };

                part->_drawcall = drawCallback;

                auto partDrawable = scene->createDrawable(*part);
                drawables.emplace_back(partDrawable.id());
            }

           model._partDrawables = drawables;
        }

        // one more drawable for the edges
        {
            auto edgesPipeline = _pipelineInspectUVMap;
            auto drawEdges = new ModelDrawableInspectorEdges();
            drawEdges->_bound = model._bound;
            // And now a render callback where we describe the rendering sequence
            graphics::DrawObjectCallback drawCallback = [pmodel, descriptorSet, edgesPipeline, numEdges, numNodes, numParts, numMaterials](
                const NodeID node,
                const graphics::CameraPointer& camera,
                const graphics::SwapchainPointer& swapchain,
                const graphics::DevicePointer& device,
                const graphics::BatchPointer& batch) {

                    if (pmodel->getUniforms()->showUVEdges) {
                        batch->bindPipeline(edgesPipeline);
                        batch->setViewport(camera->getViewportRect());
                        batch->setScissor(camera->getViewportRect());

                        batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet);

                        auto flags = pmodel->getUniforms()->buildFlags();
                        ModelObjectData odata{ (int32_t)node, (int32_t)-1, numNodes, numParts, numMaterials, numEdges, 1 | flags};
                        batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 1, sizeof(ModelObjectData), (const uint8_t*)&odata);
                        batch->draw(numEdges * 2, 0);
                    }
            };

            drawEdges->_drawcall = drawCallback;
            model._drawEdgesID = scene->createDrawable(*drawEdges).id();
        }


        // Generate the edge seam map pass
        {
            auto edgesPipeline = _pipelineMakeSeamMap;
            auto makeEdges = new ModelDrawableInspectorEdges();
            makeEdges->_bound = model._bound;
            // And now a render callback where we describe the rendering sequence
            graphics::DrawObjectCallback drawCallback = [pmodel, edgeFramebuffer, edgeMap, makeSeamDescriptorSet, edgesPipeline, numEdges, numNodes, numParts, numMaterials](
                const NodeID node,
                const graphics::CameraPointer& camera,
                const graphics::SwapchainPointer& swapchain,
                const graphics::DevicePointer& device,
                const graphics::BatchPointer& batch) {
                    static bool first{ true };
                    if (first) {
                        first = false;
                        //batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::SHADER_RESOURCE, graphics::ResourceState::RENDER_TARGET, edgeMap);

                        batch->bindFramebuffer(edgeFramebuffer);
                        batch->bindPipeline(edgesPipeline);

                        core::vec4 viewport(0, 0, edgeFramebuffer->width(), edgeFramebuffer->height());
                        batch->setViewport(viewport);
                        batch->setScissor(viewport);

                        batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, makeSeamDescriptorSet);

                        auto flags = pmodel->getUniforms()->buildFlags();
                        ModelObjectData odata{ (int32_t)node, (int32_t)-1, numNodes, numParts, numMaterials, numEdges, 1 | (uint32_t)ModelDrawableInspectorUniforms::MAKE_EDGE_MAP_BIT };
                        batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 1, sizeof(ModelObjectData), (const uint8_t*)&odata);
                        batch->draw(numEdges * 2, 0);

                        batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::RENDER_TARGET, graphics::ResourceState::SHADER_RESOURCE, edgeMap);
                        batch->beginPass(swapchain, swapchain->currentIndex());
                    }
            };

            makeEdges->_drawcall = drawCallback;
            model._makeEdgesID = scene->createDrawable(*makeEdges).id();

        }

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

        // Make edge item 
        items.emplace_back(scene->createItem(rootNode.id(), model._makeEdgesID).id());

        // first item is the model drawable
        items.emplace_back(scene->createItem(rootNode.id(), model._drawableID).id());

        // one item per parts
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

        // One more item for the edges
        items.emplace_back(scene->createItem(rootNode.id(), model._drawEdgesID).id());

        return items; 
    }



    ModelEdgeArray computeUVSeamsEdges(const ModelDrawable& model) {
        ModelEdgeArray edges;

        // detect edges on uv seams
        // same 
       
        struct VI{
            uint32_t p;
            uint32_t i;
            uint32_t v;
        };
        using VIs = std::vector<VI>;
        using VImap = std::unordered_map<size_t, VIs>;
        VImap vimap;

        using EI = ModelEdge;
        using EIs = std::vector<EI>;
        using EImap = std::unordered_map<size_t, EIs>;
        EImap edgeMap;

        uint32_t pi = 0;
        for (auto& p : model._parts) {
            if (p.attribOffset != MODEL_INVALID_INDEX) {
                for (uint32_t i = 0; i < p.numIndices; ++i) {
                    auto index = model._indices[i + p.indexOffset];
                    auto& v = model._vertices[index + p.vertexOffset];
                    auto& a = model._vertex_attribs[index + p.attribOffset];

                    //uint64_t k = *reinterpret_cast<uint64_t*>(&a.xy());
                    size_t k = *reinterpret_cast<uint64_t*>(&a.xy());

                    VI vi = {pi, i, index + p.vertexOffset };

                    auto bi = vimap.find(k);
                    if (bi == vimap.end()) {
                        vimap.insert({ k, { vi } });
                    }
                    else {
                        (*bi).second.emplace_back(vi);
                    }
                }

                for (uint32_t i = 0; i < p.numIndices; i += 3) {

                    auto i0 = model._indices[i + p.indexOffset];
                    auto i1 = model._indices[i + 1 + p.indexOffset];
                    auto i2 = model._indices[i + 2 + p.indexOffset];

                    auto& v0 = model._vertices[i0 + p.vertexOffset];
                    auto& v1 = model._vertices[i1 + p.vertexOffset];
                    auto& v2 = model._vertices[i2 + p.vertexOffset];
                    auto& a0 = model._vertex_attribs[i0 + p.attribOffset];
                    auto& a1 = model._vertex_attribs[i1 + p.attribOffset];
                    auto& a2 = model._vertex_attribs[i2 + p.attribOffset];

                    auto ai0 = i0 + p.attribOffset;
                    auto ai1 = i1 + p.attribOffset;
                    auto ai2 = i2 + p.attribOffset;

                    EI ei0 = { pi, i,     i + 1, (ai0 > ai1) };
                    EI ei1 = { pi, i + 1, i + 2, (ai1 > ai2) };
                    EI ei2 = { pi, i + 2, i,     (ai2 > ai0) };

                    //uint64_t k = *reinterpret_cast<uint64_t*>(&a.xy());
                    // size_t k = std::hash<core::vec4>(core::vec4(a0.x, a0.y, a1.x, a1.y));

                //    uint64_t k0 = *reinterpret_cast<uint64_t*>(&e0);
                    //  uint64_t k1 = *reinterpret_cast<uint64_t*>(&e1);
                    //  uint64_t k2 = *reinterpret_cast<uint64_t*>(&e2);

                    uint64_t k0 = *reinterpret_cast<uint64_t*>(&a0.xy()) | *reinterpret_cast<uint64_t*>(&a1.xy());
                    uint64_t k1 = *reinterpret_cast<uint64_t*>(&a1.xy()) | *reinterpret_cast<uint64_t*>(&a2.xy());
                    uint64_t k2 = *reinterpret_cast<uint64_t*>(&a2.xy()) | *reinterpret_cast<uint64_t*>(&a0.xy());
                    

                    auto bi0 = edgeMap.find(k0);
                    if (bi0 == edgeMap.end()) {
                        edgeMap.insert({ k0, { ei0 } });
                    } else {
                        (*bi0).second.emplace_back(ei0);
                    }
                    auto bi1 = edgeMap.find(k1);
                    if (bi1 == edgeMap.end()) {
                        edgeMap.insert({ k1, { ei1 } });
                    }
                    else {
                        (*bi1).second.emplace_back(ei1);
                    }
                    auto bi2 = edgeMap.find(k2);
                    if (bi2 == edgeMap.end()) {
                        edgeMap.insert({ k2, { ei2 } });
                    }
                    else {
                        (*bi2).second.emplace_back(ei2);
                    }
                }
            }
            pi++;
        }


        // LEt's go through the edges and keep the single ones which should be the seam edges
        // and find their matching neighboors
        for (auto& eb : edgeMap) {
            if (eb.second.size() == 1) {
                auto e = eb.second[0];
                edges.emplace_back(e);
            }
        }
        return edges;
    }

} // !namespace graphics