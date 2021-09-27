// PrimitiveDrawable.cpp
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
#include "PrimitiveDrawable.h"

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

#include "Primitive_vert.h"
#include "Primitive_frag.h"

//using namespace view3d;
namespace graphics
{

    PrimitiveDrawableFactory::PrimitiveDrawableFactory() :
        _sharedUniforms(std::make_shared<PrimitiveDrawableUniforms>()) {

    }
    PrimitiveDrawableFactory::~PrimitiveDrawableFactory() {

    }

    // Custom data uniforms
    struct PrimitiveObjectData {
        uint32_t nodeID{0};
        float numVertices{ 0 };
        float numIndices{ 0 };
        float stride{ 0 };
    };

    void PrimitiveDrawableFactory::allocateGPUShared(const graphics::DevicePointer& device) {

        // Let's describe the pipeline Descriptors layout
        graphics::RootDescriptorLayoutInit rootLayoutInit{
            {
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::VERTEX, 1, sizeof(PrimitiveObjectData) >> 2}
            },
            {{
            { graphics::DescriptorType::UNIFORM_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
            }}
         };
        auto rootDescriptorLayout = device->createRootDescriptorLayout(rootLayoutInit);

        // And a Pipeline

        // test: create shader
        graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "main", "", Primitive_vert::getSource(), Primitive_vert::getSourceFilename() };
        graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main", "", Primitive_frag::getSource(), Primitive_frag::getSourceFilename() };
        graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

        graphics::ProgramInit programInit{ vertexShader, pixelShader };
        graphics::ShaderPointer programShader = device->createProgram(programInit);

        graphics::GraphicsPipelineStateInit pipelineInit{
                    programShader,
                    rootDescriptorLayout,
                    StreamLayout(),
                    graphics::PrimitiveTopology::TRIANGLE,
                    RasterizerState(),
                    true, // enable depth
                    BlendState()
        };
        _primitivePipeline = device->createGraphicsPipelineState(pipelineInit);
    }

    graphics::PrimitiveDrawable* PrimitiveDrawableFactory::createPrimitive(const graphics::DevicePointer& device) {
        auto primitiveDrawable = new PrimitiveDrawable();
        primitiveDrawable->_uniforms = _sharedUniforms;
        return primitiveDrawable;
    }

   void PrimitiveDrawableFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        const graphics::ScenePointer& scene,
        const graphics::CameraPointer& camera,
        graphics::PrimitiveDrawable& prim)
    {
        // It s time to create a descriptorSet that matches the expected pipeline descriptor set
        // then we will assign a uniform buffer in it
        graphics::DescriptorSetInit descriptorSetInit{
            _primitivePipeline->getRootDescriptorLayout(),
            0
        };
        auto descriptorSet = device->createDescriptorSet(descriptorSetInit);

        // Assign the Camera UBO just created as the resource of the descriptorSet
        graphics::DescriptorObjects descriptorObjects = {
            { graphics::DescriptorType::UNIFORM_BUFFER, camera->getGPUBuffer() },
            { graphics::DescriptorType::RESOURCE_BUFFER, scene->_nodes._transforms_buffer }
        };
        device->updateDescriptorSet(descriptorSet, descriptorObjects);

        auto prim_ = &prim;
        auto pipeline = this->_primitivePipeline;

        // And now a render callback where we describe the rendering sequence
        graphics::DrawObjectCallback drawCallback = [prim_, descriptorSet, pipeline](const NodeID node, const graphics::CameraPointer& camera, const graphics::SwapchainPointer& swapchain, const graphics::DevicePointer& device, const graphics::BatchPointer& batch) {
            batch->bindPipeline(pipeline);
            batch->setViewport(camera->getViewportRect());
            batch->setScissor(camera->getViewportRect());

            batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet);

            PrimitiveObjectData odata{ node, prim_->_size.x * 0.5f, prim_->_size.y * 0.5f, prim_->_size.z * 0.5f };
            batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(PrimitiveObjectData), (const uint8_t*)&odata);

            // A box is 6 faces * 2 trianglestrip * 4 verts + -1
            batch->draw(6 * 2 * 3, 0);
        };
        prim._drawcall = drawCallback;
    }

} // !namespace graphics