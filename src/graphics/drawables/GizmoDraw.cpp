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

    GizmoDrawFactory::GizmoDrawFactory(const graphics::DevicePointer& device) :
        _sharedUniforms(std::make_shared<GizmoDrawUniforms>()) {

        allocateGPUShared(device);
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

    void GizmoDrawFactory::allocateGPUShared(const graphics::DevicePointer& device) {
        // Let's describe the pipeline Descriptors for _node
        graphics::RootDescriptorLayoutInit rootDescriptorLayoutInit{
            {
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::VERTEX, 1, sizeof(GizmoObjectData) >> 2},
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
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 1, 1},
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 2, 1},
                }
                );

        auto rootDescriptorLayout_item = device->createRootDescriptorLayout(rootDescriptorLayoutInit);

        // And a Pipeline


        // test: create shader
        graphics::ShaderIncludeLib include = {
            Transform_inc::getMapEntry(),
            Projection_inc::getMapEntry(),
            Camera_inc::getMapEntry(),
            SceneTransform_inc::getMapEntry()
        };


        graphics::ShaderInit vertexShaderInit_node{ graphics::ShaderType::VERTEX, "main", GizmoNode_vert::getSource, GizmoNode_vert::getSourceFilename(), include };
        graphics::ShaderPointer vertexShader_node = device->createShader(vertexShaderInit_node);

        graphics::ShaderInit vertexShaderInit_item{ graphics::ShaderType::VERTEX, "main", GizmoItem_vert::getSource, GizmoItem_vert::getSourceFilename(), include };
        graphics::ShaderPointer vertexShader_item = device->createShader(vertexShaderInit_item);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main", Gizmo_frag::getSource, Gizmo_frag::getSourceFilename(), include };
        graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

        graphics::ProgramInit programInit_node{ vertexShader_node, pixelShader };
        graphics::ShaderPointer programShader_node = device->createProgram(programInit_node);

        graphics::ProgramInit programInit_item{ vertexShader_item, pixelShader };
        graphics::ShaderPointer programShader_item = device->createProgram(programInit_item);

        graphics::GraphicsPipelineStateInit pipelineInit_node{
                    programShader_node,
                    rootDescriptorLayout_node,
                    StreamLayout(),
                    graphics::PrimitiveTopology::LINE,
                    RasterizerState(),
                    true, // enable depth
                    BlendState()
        };
        _nodePipeline = device->createGraphicsPipelineState(pipelineInit_node);

        graphics::GraphicsPipelineStateInit pipelineInit_item{
                    programShader_item,
                    rootDescriptorLayout_item,
                    StreamLayout(),
                    graphics::PrimitiveTopology::LINE,
                    RasterizerState(),
                    true, // enable depth
                    BlendState()
        };
        _itemPipeline = device->createGraphicsPipelineState(pipelineInit_item);
    }

    graphics::NodeGizmo* GizmoDrawFactory::createNodeGizmo(const graphics::DevicePointer& device) {

        auto gizmoDraw = new NodeGizmo();

        // Create the triangle soup draw using the shared uniforms of the factory
        gizmoDraw->_uniforms = _sharedUniforms;

        return gizmoDraw;
    }

    graphics::ItemGizmo* GizmoDrawFactory::createItemGizmo(const graphics::DevicePointer& device) {

        auto gizmoDraw = new ItemGizmo();

        // Create the triangle soup draw using the shared uniforms of the factory
        gizmoDraw->_uniforms = _sharedUniforms;

        return gizmoDraw;
    }

    graphics::CameraGizmo* GizmoDrawFactory::createCameraGizmo(const graphics::DevicePointer& device) {

        auto gizmoDraw = new CameraGizmo();

        // Create the triangle soup draw using the shared uniforms of the factory
        gizmoDraw->_uniforms = _sharedUniforms;

        return gizmoDraw;
    }

    void GizmoDrawFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        const graphics::ScenePointer& scene,
        graphics::NodeGizmo& gizmo)
    {
        auto pgizmo = &gizmo;
        auto pipeline = this->_nodePipeline;

        // And now a render callback where we describe the rendering sequence
        graphics::DrawObjectCallback drawCallback = [pgizmo, pipeline](const NodeID node, RenderArgs& args) {
            args.batch->bindPipeline(pipeline);
            args.batch->setViewport(args.camera->getViewportRect());
            args.batch->setScissor(args.camera->getViewportRect());

            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);

            const auto& uni = *pgizmo->getUniforms();
            auto count = core::min(uni.indexCount, args.scene->_nodes.numAllocatedNodes() - uni.indexOffset);
            auto flags = uni.buildFlags();
            GizmoObjectData odata{ uni.indexOffset, flags, 0, 0};
            args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(GizmoObjectData), (const uint8_t*)&odata);

            args.batch->draw(count * 2 * ((flags & GizmoDrawUniforms::SHOW_TRANSFORM_BIT) * 3 + (flags & GizmoDrawUniforms::SHOW_BRANCH_BIT) * 1), 0);
        };
        gizmo._drawcall = drawCallback;
    }


   void GizmoDrawFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        const graphics::ScenePointer& scene,
        graphics::ItemGizmo& gizmo)
   {
       // Create DescriptorSet #1, #0 is ViewPassDS
       graphics::DescriptorSetInit descriptorSetInit{
           _itemPipeline->getRootDescriptorLayout(),
            1
       };
       auto descriptorSet = device->createDescriptorSet(descriptorSetInit);

       graphics::DescriptorObjects descriptorObjects = {{
            { graphics::DescriptorType::RESOURCE_BUFFER, scene->_drawables._drawables_buffer },
            { graphics::DescriptorType::RESOURCE_BUFFER, scene->_items.getGPUBuffer()},

       }};
       device->updateDescriptorSet(descriptorSet, descriptorObjects);

       auto pgizmo = &gizmo;
       auto pipeline = this->_itemPipeline;

       // And now a render callback where we describe the rendering sequence
       graphics::DrawObjectCallback drawCallback = [pgizmo, descriptorSet, pipeline](const NodeID node, RenderArgs& args) {
           
           args.batch->bindPipeline(pipeline);
           args.batch->setViewport(args.camera->getViewportRect());
           args.batch->setScissor(args.camera->getViewportRect());

           args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
           args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet);

           const auto& uni = *pgizmo->getUniforms();
           auto count = core::min(uni.indexCount, args.scene->_items.numAllocatedItems() - uni.indexOffset);
           auto flags = uni.buildFlags();
           GizmoObjectData odata{ uni.indexOffset, flags, 0, 0 };
           args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(GizmoObjectData), (const uint8_t*)&odata);

           args.batch->draw(count * 2 * ((flags & GizmoDrawUniforms::SHOW_LOCAL_BOUND_BIT) * 12 + (flags & GizmoDrawUniforms::SHOW_WORLD_BOUND_BIT) * 12), 0);
       };
       gizmo._drawcall = drawCallback;
   }

   void GizmoDrawFactory::allocateDrawcallObject(
       const graphics::DevicePointer& device,
       const graphics::ScenePointer& scene,
       graphics::CameraGizmo& gizmo)
   {
       // Create DescriptorSet #1, #0 is ViewPassDS
       graphics::DescriptorSetInit descriptorSetInit{
           _itemPipeline->getRootDescriptorLayout(),
            1
       };
       auto descriptorSet = device->createDescriptorSet(descriptorSetInit);

       graphics::DescriptorObjects descriptorObjects = { {
            { graphics::DescriptorType::RESOURCE_BUFFER, scene->_drawables._drawables_buffer },
            { graphics::DescriptorType::RESOURCE_BUFFER, scene->_items.getGPUBuffer()},

       } };
       device->updateDescriptorSet(descriptorSet, descriptorObjects);

       auto pgizmo = &gizmo;
       auto pipeline = this->_itemPipeline;

       // And now a render callback where we describe the rendering sequence
       graphics::DrawObjectCallback drawCallback = [pgizmo, descriptorSet, pipeline](const NodeID node, RenderArgs& args) {

           args.batch->bindPipeline(pipeline);
           args.batch->setViewport(args.camera->getViewportRect());
           args.batch->setScissor(args.camera->getViewportRect());

           args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
           args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet);

           const auto& uni = *pgizmo->getUniforms();
           auto count = core::min(uni.indexCount, args.scene->_items.numAllocatedItems() - uni.indexOffset);
           auto flags = uni.buildFlags();
           GizmoObjectData odata{ uni.indexOffset, flags, 0, 0 };
           args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(GizmoObjectData), (const uint8_t*)&odata);

           args.batch->draw(count * 2 * ((flags & GizmoDrawUniforms::SHOW_LOCAL_BOUND_BIT) * 12 + (flags & GizmoDrawUniforms::SHOW_WORLD_BOUND_BIT) * 12), 0);
       };
       gizmo._drawcall = drawCallback;
   }

   std::tuple<graphics::Item, graphics::Item> GizmoDraw_createSceneGizmos(const ScenePointer& scene, const DevicePointer& gpuDevice) {
       // A gizmo draw factory
       auto gizmoDrawFactory = std::make_shared<GizmoDrawFactory>(gpuDevice);

       // a gizmo draw to draw the transforms
       auto node_tree_draw = scene->createDraw(*gizmoDrawFactory->createNodeGizmo(gpuDevice));
       gizmoDrawFactory->allocateDrawcallObject(gpuDevice, scene, node_tree_draw.as<NodeGizmo>());
       auto node_tree = scene->createItem(graphics::Node::null, node_tree_draw);
       node_tree.setVisible(true);


       auto item_tree_draw = scene->createDraw(*gizmoDrawFactory->createItemGizmo(gpuDevice));
       gizmoDrawFactory->allocateDrawcallObject(gpuDevice, scene, item_tree_draw.as<ItemGizmo>());
       auto item_tree = scene->createItem(graphics::Node::null, item_tree_draw);
       item_tree.setVisible(true);

       return std::tuple<graphics::Item, graphics::Item>{node_tree, item_tree};
   }

} // !namespace graphics