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
        uint32_t numVertices{ 0 };
        uint32_t numIndices{ 0 };
        uint32_t stride{ 0 };
        float triangleScale { 0.1f };
    };

    void GizmoDrawableFactory::allocateGPUShared(const graphics::DevicePointer& device) {

        // Let's describe the pipeline Descriptors layout
        graphics::DescriptorLayouts descriptorLayouts_node{
            { graphics::DescriptorType::UNIFORM_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::VERTEX, 1, sizeof(GizmoObjectData) >> 2},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
        };
        graphics::DescriptorLayouts descriptorLayouts_item{
            { graphics::DescriptorType::UNIFORM_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::VERTEX, 1, sizeof(GizmoObjectData) >> 2},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 1, 1},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 2, 1},
        };

        graphics::DescriptorSetLayoutInit descriptorSetLayoutInit_node{ descriptorLayouts_node };
        auto descriptorSetLayout_node = device->createDescriptorSetLayout(descriptorSetLayoutInit_node);
        graphics::DescriptorSetLayoutInit descriptorSetLayoutInit_item{ descriptorLayouts_item };
        auto descriptorSetLayout_item = device->createDescriptorSetLayout(descriptorSetLayoutInit_item);

        // And a Pipeline

        auto shader_pixel_src = Gizmo_frag::getSource();

        // test: create shader
        graphics::ShaderInit vertexShaderInit_node{ graphics::ShaderType::VERTEX, "main", "", GizmoNode_vert::getSource(), GizmoNode_vert::getSourceFilename() };
        graphics::ShaderPointer vertexShader_node = device->createShader(vertexShaderInit_node);

        graphics::ShaderInit vertexShaderInit_item{ graphics::ShaderType::VERTEX, "main", "", GizmoItem_vert::getSource(), GizmoItem_vert::getSourceFilename() };
        graphics::ShaderPointer vertexShader_item = device->createShader(vertexShaderInit_item);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main", "", shader_pixel_src, Gizmo_frag::getSourceFilename() };
        graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

        graphics::ProgramInit programInit_node{ vertexShader_node, pixelShader };
        graphics::ShaderPointer programShader_node = device->createProgram(programInit_node);

        graphics::ProgramInit programInit_item{ vertexShader_item, pixelShader };
        graphics::ShaderPointer programShader_item = device->createProgram(programInit_item);

        graphics::PipelineStateInit pipelineInit_node{
                    programShader_node,
                    StreamLayout(),
                    graphics::PrimitiveTopology::LINE,
                    descriptorSetLayout_node,
                    RasterizerState(),
                    true, // enable depth
                    BlendState()
        };
        _nodePipeline = device->createPipelineState(pipelineInit_node);

        graphics::PipelineStateInit pipelineInit_item{
                    programShader_item,
                    StreamLayout(),
                    graphics::PrimitiveTopology::LINE,
                    descriptorSetLayout_item,
                    RasterizerState(),
                    true, // enable depth
                    BlendState()
        };
        _itemPipeline = device->createPipelineState(pipelineInit_item);
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
        const graphics::CameraPointer& camera,
        graphics::NodeGizmo& gizmo)
    {
        // It s time to create a descriptorSet that matches the expected pipeline descriptor set
        // then we will assign a uniform buffer in it
        graphics::DescriptorSetInit descriptorSetInit{
            _nodePipeline->getDescriptorSetLayout()
        };
        auto descriptorSet = device->createDescriptorSet(descriptorSetInit);

        // Assign the Camera UBO just created as the resource of the descriptorSet
        // auto descriptorObjects = descriptorSet->buildDescriptorObjects();
        graphics::DescriptorObject camera_uboDescriptorObject;
        camera_uboDescriptorObject._uniformBuffers.push_back(camera->getGPUBuffer());
        graphics::DescriptorObject transform_rboDescriptorObject;
        transform_rboDescriptorObject._buffers.push_back(scene->_nodes._transforms_buffer);
        graphics::DescriptorObjects descriptorObjects = {
            camera_uboDescriptorObject,
            transform_rboDescriptorObject
        };
        device->updateDescriptorSet(descriptorSet, descriptorObjects);

        auto pgizmo = &gizmo;
        auto pipeline = this->_nodePipeline;

        // And now a render callback where we describe the rendering sequence
        graphics::DrawObjectCallback drawCallback = [pgizmo, descriptorSet, pipeline](const NodeID node, const graphics::CameraPointer& camera, const graphics::SwapchainPointer& swapchain, const graphics::DevicePointer& device, const graphics::BatchPointer& batch) {
            batch->setPipeline(pipeline);
            batch->setViewport(camera->getViewportRect());
            batch->setScissor(camera->getViewportRect());

            batch->bindDescriptorSet(descriptorSet);

            GizmoObjectData odata{ node, 0, 0, 0, pgizmo->getUniforms()->triangleScale };
            batch->bindPushUniform(1, sizeof(GizmoObjectData), (const uint8_t*)&odata);

            batch->draw(pgizmo->nodes.size() * 2 * (3 + 1), 0);
        };
        gizmo._drawcall = drawCallback;
    }


   void GizmoDrawableFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        const graphics::ScenePointer& scene,
        const graphics::CameraPointer& camera,
        graphics::ItemGizmo& gizmo)
   {
       // It s time to create a descriptorSet that matches the expected pipeline descriptor set
       // then we will assign a uniform buffer in it
       graphics::DescriptorSetInit descriptorSetInit{
           _itemPipeline->getDescriptorSetLayout()
       };
       auto descriptorSet = device->createDescriptorSet(descriptorSetInit);

       // Assign the Camera UBO just created as the resource of the descriptorSet
       // auto descriptorObjects = descriptorSet->buildDescriptorObjects();
       graphics::DescriptorObject camera_uboDescriptorObject;
       camera_uboDescriptorObject._uniformBuffers.push_back(camera->getGPUBuffer());
       graphics::DescriptorObject items_rboDescriptorObject;
       items_rboDescriptorObject._buffers.push_back(scene->_items._items_buffer);
       graphics::DescriptorObject transform_rboDescriptorObject;
       transform_rboDescriptorObject._buffers.push_back(scene->_nodes._transforms_buffer);
       graphics::DescriptorObject drawables_rboDescriptorObject;
       drawables_rboDescriptorObject._buffers.push_back(scene->_drawables._drawables_buffer);

       graphics::DescriptorObjects descriptorObjects = {
            camera_uboDescriptorObject,
            transform_rboDescriptorObject,
            drawables_rboDescriptorObject,
            items_rboDescriptorObject
       };
       device->updateDescriptorSet(descriptorSet, descriptorObjects);

       auto pgizmo = &gizmo;
       auto pipeline = this->_itemPipeline;

       // And now a render callback where we describe the rendering sequence
       graphics::DrawObjectCallback drawCallback = [pgizmo, descriptorSet, pipeline](const NodeID node, const graphics::CameraPointer& camera, const graphics::SwapchainPointer& swapchain, const graphics::DevicePointer& device, const graphics::BatchPointer& batch) {
           batch->setPipeline(pipeline);
           batch->setViewport(camera->getViewportRect());
           batch->setScissor(camera->getViewportRect());

           batch->bindDescriptorSet(descriptorSet);

           GizmoObjectData odata{ node, 0, 0, 0, pgizmo->getUniforms()->triangleScale };
           batch->bindPushUniform(1, sizeof(GizmoObjectData), (const uint8_t*)&odata);

           batch->draw(pgizmo->items.size() * 2 * (12 + 12), 0);
       };
       gizmo._drawcall = drawCallback;
   }

} // !namespace graphics