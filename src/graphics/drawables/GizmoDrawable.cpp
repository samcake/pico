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

#include "Gizmo_vert.h"
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
        graphics::DescriptorLayouts descriptorLayouts{
            { graphics::DescriptorType::UNIFORM_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::VERTEX, 1, sizeof(GizmoObjectData) >> 2},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 1, 1},
        };

        graphics::DescriptorSetLayoutInit descriptorSetLayoutInit{ descriptorLayouts };
        auto descriptorSetLayout = device->createDescriptorSetLayout(descriptorSetLayoutInit);

        // And a Pipeline

        // Load shaders (as stored in the resources)
        auto shader_vertex_src = Gizmo_vert::getSource();
        auto shader_pixel_src = Gizmo_frag::getSource();

        // test: create shader
        graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "main", "", shader_vertex_src, Gizmo_vert::getSourceFilename() };
        graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main", "", shader_pixel_src, Gizmo_frag::getSourceFilename() };
        graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

        graphics::ProgramInit programInit{ vertexShader, pixelShader };
        graphics::ShaderPointer programShader = device->createProgram(programInit);

        graphics::PipelineStateInit pipelineInit{
                    programShader,
                    StreamLayout(),
                    graphics::PrimitiveTopology::LINE,
                    descriptorSetLayout,
                    RasterizerState(),
                    true, // enable depth
                    BlendState()
        };
        _pipeline = device->createPipelineState(pipelineInit);
    }

    graphics::GizmoDrawable* GizmoDrawableFactory::createGizmoDrawable(const graphics::DevicePointer& device) {

        auto gizmoDrawable = new GizmoDrawable();

        // Create the triangle soup drawable using the shared uniforms of the factory
        gizmoDrawable->_uniforms = _sharedUniforms;

        return gizmoDrawable;
    }

    graphics::DrawcallObjectPointer GizmoDrawableFactory::allocateDrawcallObject(const graphics::DevicePointer& device,
        const graphics::TransformTreeGPUPointer& transform,
        const graphics::CameraPointer& camera,
        const graphics::GizmoDrawablePointer& gizmo)
    {
        // It s time to create a descriptorSet that matches the expected pipeline descriptor set
        // then we will assign a uniform buffer in it
        graphics::DescriptorSetInit descriptorSetInit{
            _pipeline->getDescriptorSetLayout()
        };
        auto descriptorSet = device->createDescriptorSet(descriptorSetInit);

        // Assign the Camera UBO just created as the resource of the descriptorSet
        // auto descriptorObjects = descriptorSet->buildDescriptorObjects();
        graphics::DescriptorObject camera_uboDescriptorObject;
        camera_uboDescriptorObject._uniformBuffers.push_back(camera->getGPUBuffer());
        graphics::DescriptorObject transform_rboDescriptorObject;
        transform_rboDescriptorObject._buffers.push_back(transform->_transforms_buffer);
        graphics::DescriptorObject bound_rboDescriptorObject;
        bound_rboDescriptorObject._buffers.push_back(transform->_bounds_buffer);
        graphics::DescriptorObjects descriptorObjects = {
            camera_uboDescriptorObject,
            transform_rboDescriptorObject,
            bound_rboDescriptorObject
        };
        device->updateDescriptorSet(descriptorSet, descriptorObjects);

        // And now a render callback where we describe the rendering sequence
        graphics::DrawObjectCallback drawCallback = [gizmo, descriptorSet, this](const NodeID node, const graphics::CameraPointer& camera, const graphics::SwapchainPointer& swapchain, const graphics::DevicePointer& device, const graphics::BatchPointer& batch) {
            batch->setPipeline(this->_pipeline);
            batch->setViewport(camera->getViewportRect());
            batch->setScissor(camera->getViewportRect());

            batch->bindDescriptorSet(descriptorSet);

            auto uniforms = gizmo->getUniforms();
            GizmoObjectData odata{ node, 0, 0, 0, uniforms->triangleScale };
            batch->bindPushUniform(1, sizeof(GizmoObjectData), (const uint8_t*)&odata);

            batch->draw(gizmo->nodes.size() * 2 * (3 + 1 + 12), 0);
        };
        auto drawcall = new graphics::DrawcallObject(drawCallback);
        gizmo->_drawcall = graphics::DrawcallObjectPointer(drawcall);

        return gizmo->_drawcall;
    }


    GizmoDrawable::GizmoDrawable() {

    }
    GizmoDrawable::~GizmoDrawable() {

    }
    void GizmoDrawable::setNode(graphics::NodeID node) const {
        _nodeID = node;
    }
    graphics::DrawcallObjectPointer GizmoDrawable::getDrawable() const {
        return _drawcall;
    }

} // !namespace graphics