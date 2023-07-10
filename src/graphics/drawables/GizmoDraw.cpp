// GizmoDraw.cpp
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
#include "GizmoDraw.h"

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
#include "render/Draw.h"
#include "render/Viewport.h"
#include "render/Mesh.h"

#include "GizmoNode_vert.h"
#include "GizmoItem_vert.h"
#include "Gizmo_frag.h"

#include "Transform_inc.h"
#include "Projection_inc.h"
#include "Camera_inc.h"
#include "SceneTransform_inc.h"

//using namespace view3d;
namespace graphics
{

    GizmoDrawFactory::GizmoDrawFactory(const DevicePointer& device, const ScenePointer& scene) :
        _sharedUniforms(std::make_shared<GizmoDrawUniforms>()) {

        allocateGPUShared(device, scene);
    }
    GizmoDrawFactory::~GizmoDrawFactory() {

    }

    // Custom data uniforms
    struct GizmoObjectData {
        uint32_t indexOffset{0};
        uint32_t flags{0};
        uint32_t spareA{ 0 };
        uint32_t spareB{0};
    };

    uint32_t GizmoDrawUniforms::buildFlags() const {
        return (uint32_t) 
                  (showTransform) * SHOW_TRANSFORM_BIT 
                | (showBranch) * SHOW_BRANCH_BIT
                | (showLocalBound) * SHOW_LOCAL_BOUND_BIT
                | (showWorldBound)  *SHOW_WORLD_BOUND_BIT;
    }

    void GizmoDrawFactory::allocateGPUShared(const DevicePointer& device, const ScenePointer& scene) {
        // Let's describe the pipeline Descriptors for _node
        RootDescriptorLayoutInit rootDescriptorLayoutInit{
            {
            { DescriptorType::PUSH_UNIFORM, ShaderStage::VERTEX, 1, sizeof(GizmoObjectData) >> 2},
            },
            {
                // ViewPass descriptorSet Layout
                Viewport::viewPassLayout
            }
        };
        auto rootDescriptorLayout_node = device->createRootDescriptorLayout(rootDescriptorLayoutInit);

        // Let's describe the pipeline Descriptors layout same as _node + another descriptorset
        rootDescriptorLayoutInit._setLayouts.push_back(
                {
                { DescriptorType::RESOURCE_BUFFER, ShaderStage::VERTEX, 1, 1},
                { DescriptorType::RESOURCE_BUFFER, ShaderStage::VERTEX, 2, 1},
                }
                );

        auto rootDescriptorLayout_item = device->createRootDescriptorLayout(rootDescriptorLayoutInit);

        // And a Pipeline


        // test: create shader
        ShaderIncludeLib include = {
            Transform_inc::getMapEntry(),
            Projection_inc::getMapEntry(),
            Camera_inc::getMapEntry(),
            SceneTransform_inc::getMapEntry()
        };


        ShaderInit vertexShaderInit_node{ ShaderType::VERTEX, "main", GizmoNode_vert::getSource, GizmoNode_vert::getSourceFilename(), include };
        ShaderPointer vertexShader_node = device->createShader(vertexShaderInit_node);

        ShaderInit vertexShaderInit_item{ ShaderType::VERTEX, "main", GizmoItem_vert::getSource, GizmoItem_vert::getSourceFilename(), include };
        ShaderPointer vertexShader_item = device->createShader(vertexShaderInit_item);

        ShaderInit pixelShaderInit{ ShaderType::PIXEL, "main", Gizmo_frag::getSource, Gizmo_frag::getSourceFilename(), include };
        ShaderPointer pixelShader = device->createShader(pixelShaderInit);

        ProgramInit programInit_node{ vertexShader_node, pixelShader };
        ShaderPointer programShader_node = device->createProgram(programInit_node);

        ProgramInit programInit_item{ vertexShader_item, pixelShader };
        ShaderPointer programShader_item = device->createProgram(programInit_item);

        GraphicsPipelineStateInit pipelineInit_node{
                    programShader_node,
                    rootDescriptorLayout_node,
                    StreamLayout(),
                    PrimitiveTopology::LINE,
                    RasterizerState(),
                    { true }, // enable depth
                    BlendState()
        };
        _nodePipeline = device->createGraphicsPipelineState(pipelineInit_node);

        GraphicsPipelineStateInit pipelineInit_item{
                    programShader_item,
                    rootDescriptorLayout_item,
                    StreamLayout(),
                    PrimitiveTopology::LINE,
                    RasterizerState(),
                    { true }, // enable depth
                    BlendState()
        };
        _itemPipeline = device->createGraphicsPipelineState(pipelineInit_item);

    }

    NodeGizmo GizmoDrawFactory::createNodeGizmo(const DevicePointer& device) {

        NodeGizmo gizmoDraw;

        // Create the triangle soup draw using the shared uniforms of the factory
        gizmoDraw._uniforms = _sharedUniforms;

        allocateDrawcallObject(device, gizmoDraw);

        return gizmoDraw;
    }

    ItemGizmo GizmoDrawFactory::createItemGizmo(const DevicePointer& device, const ScenePointer& scene) {

        ItemGizmo gizmoDraw;

        // Create the triangle soup draw using the shared uniforms of the factory
        gizmoDraw._uniforms = _sharedUniforms;

        allocateDrawcallObject(device, scene, gizmoDraw);
        
        return gizmoDraw;
    }

    void GizmoDrawFactory::allocateDrawcallObject(
        const DevicePointer& device,
        NodeGizmo& gizmo)
    {
        auto pipeline = this->_nodePipeline;

        // And now a render callback where we describe the rendering sequence
        gizmo._drawcall = [pg = gizmo, pipeline](const NodeID node, RenderArgs& args) {
            args.batch->bindPipeline(pipeline);

            args.batch->bindDescriptorSet(PipelineType::GRAPHICS, args.viewPassDescriptorSet);

            const auto& uni = *pg.getUniforms();
            auto count = core::min(uni.indexCount, args.scene->_nodes.numAllocatedNodes() - uni.indexOffset);
            auto flags = uni.buildFlags();
            GizmoObjectData odata{ uni.indexOffset, flags, 0, 0};
            args.batch->bindPushUniform(PipelineType::GRAPHICS, 0, sizeof(GizmoObjectData), (const uint8_t*)&odata);

            args.batch->draw(count * 2 * ((flags & GizmoDrawUniforms::SHOW_TRANSFORM_BIT) * 3 + (flags & GizmoDrawUniforms::SHOW_BRANCH_BIT) * 1), 0);
        };
    }


   void GizmoDrawFactory::allocateDrawcallObject(
        const DevicePointer& device,
        const ScenePointer& scene,
        ItemGizmo& gizmo)
   {
       auto pgizmo = gizmo;
       auto pipeline = this->_itemPipeline;

       // Create DescriptorSet #1, #0 is ViewPassDS
       DescriptorSetInit descriptorSetInit{
           _itemPipeline->getRootDescriptorLayout(),
            1
       };

       _descriptorSet = device->createDescriptorSet(descriptorSetInit);

       DescriptorObjects descriptorObjects = { {
            { DescriptorType::RESOURCE_BUFFER, scene->_drawables.getGPUBuffer()},
            { DescriptorType::RESOURCE_BUFFER, scene->_items.getGPUBuffer()},

       } };
       device->updateDescriptorSet(_descriptorSet, descriptorObjects);

       auto descriptorSet = this->_descriptorSet;

       // And now a render callback where we describe the rendering sequence
       gizmo._drawcall = [pgizmo, descriptorSet, pipeline](const NodeID node, RenderArgs& args) {
           
           args.batch->bindPipeline(pipeline);

           args.batch->bindDescriptorSet(PipelineType::GRAPHICS, args.viewPassDescriptorSet);
           args.batch->bindDescriptorSet(PipelineType::GRAPHICS, descriptorSet);

           const auto& uni = *pgizmo.getUniforms();
           auto count = core::min(uni.indexCount, args.scene->_items.numAllocatedItems() - uni.indexOffset);
           auto flags = uni.buildFlags();
           GizmoObjectData odata{ uni.indexOffset, flags, 0, 0 };
           args.batch->bindPushUniform(PipelineType::GRAPHICS, 0, sizeof(GizmoObjectData), (const uint8_t*)&odata);

           args.batch->draw(count * 2 * ((flags & GizmoDrawUniforms::SHOW_LOCAL_BOUND_BIT) * 12 + (flags & GizmoDrawUniforms::SHOW_WORLD_BOUND_BIT) * 12), 0);
       };
   }

   std::tuple<Item, Item> GizmoDraw_createSceneGizmos(const ScenePointer& scene, const DevicePointer& gpuDevice) {
       // A gizmo draw factory
       auto gizmoDrawFactory = std::make_unique<GizmoDrawFactory>(gpuDevice, scene);

       // a gizmo draw to draw the transforms
       auto node_tree_draw = scene->createDraw(gizmoDrawFactory->createNodeGizmo(gpuDevice));
       auto node_tree = scene->createItem(Node::null, node_tree_draw);
       node_tree.setVisible(false);


       auto item_tree_draw = scene->createDraw(gizmoDrawFactory->createItemGizmo(gpuDevice, scene));
       auto item_tree = scene->createItem(Node::null, item_tree_draw);
       item_tree.setVisible(false);

       return std::tuple<Item, Item>{node_tree, item_tree};
   }

} // !namespace graphics