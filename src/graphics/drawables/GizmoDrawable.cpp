// GizmoDrawable.cpp
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
#include "GizmoDrawable.h"

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

#include "GizmoNode_vert.h"
#include "GizmoItem_vert.h"
#include "Gizmo_frag.h"

//using namespace view3d;
namespace graphics
{

    GizmoDrawableFactory::GizmoDrawableFactory() :
        _sharedUniforms(std::make_shared<GizmoDrawableUniforms>()) {

    }
    GizmoDrawableFactory::~GizmoDrawableFactory() {

    }

    // Custom data uniforms
    struct GizmoObjectData {
        uint32_t nodeID{0};
        uint32_t flags{0};
        uint32_t spareA{ 0 };
        uint32_t spareB{0};
    };

    uint32_t GizmoDrawableUniforms::buildFlags() {
        return (uint32_t) 
                  (showTransform) * SHOW_TRANSFORM_BIT 
                | (showBranch) * SHOW_BRANCH_BIT
                | (showLocalBound) * SHOW_LOCAL_BOUND_BIT
                | (showWorldBound)  *SHOW_WORLD_BOUND_BIT;
    }

    void GizmoDrawableFactory::allocateGPUShared(const graphics::DevicePointer& device) {
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
        graphics::ShaderInit vertexShaderInit_node{ graphics::ShaderType::VERTEX, "main", GizmoNode_vert::getSource, GizmoNode_vert::getSourceFilename() };
        graphics::ShaderPointer vertexShader_node = device->createShader(vertexShaderInit_node);

        graphics::ShaderInit vertexShaderInit_item{ graphics::ShaderType::VERTEX, "main", GizmoItem_vert::getSource, GizmoItem_vert::getSourceFilename() };
        graphics::ShaderPointer vertexShader_item = device->createShader(vertexShaderInit_item);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main", Gizmo_frag::getSource, Gizmo_frag::getSourceFilename() };
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

    graphics::NodeGizmo* GizmoDrawableFactory::createNodeGizmo(const graphics::DevicePointer& device) {

        auto gizmoDrawable = new NodeGizmo();

        // Create the triangle soup drawable using the shared uniforms of the factory
        gizmoDrawable->_uniforms = _sharedUniforms;

        return gizmoDrawable;
    }

    graphics::ItemGizmo* GizmoDrawableFactory::createItemGizmo(const graphics::DevicePointer& device) {

        auto gizmoDrawable = new ItemGizmo();

        // Create the triangle soup drawable using the shared uniforms of the factory
        gizmoDrawable->_uniforms = _sharedUniforms;

        return gizmoDrawable;
    }

   void GizmoDrawableFactory::allocateDrawcallObject(
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

            auto flags = pgizmo->getUniforms()->buildFlags();
            GizmoObjectData odata{ node, flags, 0, 0};
            args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(GizmoObjectData), (const uint8_t*)&odata);

            args.batch->draw(pgizmo->nodes.size() * 2 * ((flags & GizmoDrawableUniforms::SHOW_TRANSFORM_BIT) * 3 + (flags & GizmoDrawableUniforms::SHOW_BRANCH_BIT) * 1), 0);
        };
        gizmo._drawcall = drawCallback;
    }


   void GizmoDrawableFactory::allocateDrawcallObject(
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
            { graphics::DescriptorType::RESOURCE_BUFFER, scene->_items._items_buffer },

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

           auto flags = pgizmo->getUniforms()->buildFlags();
           GizmoObjectData odata{ node, flags, 0, 0 };
           args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(GizmoObjectData), (const uint8_t*)&odata);

           args.batch->draw(pgizmo->items.size() * 2 * ((flags & GizmoDrawableUniforms::SHOW_LOCAL_BOUND_BIT) * 12 + (flags & GizmoDrawableUniforms::SHOW_WORLD_BOUND_BIT) * 12), 0);
       };
       gizmo._drawcall = drawCallback;
   }

} // !namespace graphics