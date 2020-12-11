// HeightmapDrawable.cpp
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
#include "HeightmapDrawable.h"

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

#include "Heightmap_vert.h"
#include "Heightmap_frag.h"

//using namespace view3d;
namespace graphics
{

    HeightmapDrawableFactory::HeightmapDrawableFactory() :
        _sharedUniforms(std::make_shared<HeightmapDrawableUniforms>()) {

    }
    HeightmapDrawableFactory::~HeightmapDrawableFactory() {

    }

    // Custom data uniforms
    struct HeightmapObjectData {
        uint32_t nodeID{0};
        uint32_t width{ 0 };
        uint32_t height{ 0 };
        float spacing{ 0 };
    };

    void HeightmapDrawableFactory::allocateGPUShared(const graphics::DevicePointer& device) {

        // Let's describe the pipeline Descriptors layout
        graphics::DescriptorLayouts descriptorLayouts{
            { graphics::DescriptorType::UNIFORM_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::VERTEX, 1, sizeof(HeightmapObjectData) >> 2},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
        };

        graphics::DescriptorSetLayoutInit descriptorSetLayoutInit{ descriptorLayouts };
        auto descriptorSetLayout = device->createDescriptorSetLayout(descriptorSetLayoutInit);

        // And a Pipeline

        // test: create shader
        graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "main", "", Heightmap_vert::getSource(), Heightmap_vert::getSourceFilename() };
        graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main", "", Heightmap_frag::getSource(), Heightmap_frag::getSourceFilename() };
        graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

        graphics::ProgramInit programInit{ vertexShader, pixelShader };
        graphics::ShaderPointer programShader = device->createProgram(programInit);

        graphics::PipelineStateInit pipelineInit{
                    programShader,
                    StreamLayout(),
                    graphics::PrimitiveTopology::TRIANGLE_STRIP,
                    descriptorSetLayout,
                    RasterizerState(),
                    true, // enable depth
                    BlendState()
        };
        pipelineInit.rasterizer.fillMode = graphics::FillMode::LINE;
        _HeightmapPipeline = device->createPipelineState(pipelineInit);
    }

    graphics::HeightmapDrawable* HeightmapDrawableFactory::createHeightmap(const graphics::DevicePointer& device, const Heightmap& heightmap) {
        auto HeightmapDrawable = new graphics::HeightmapDrawable();
        HeightmapDrawable->_heightmap = heightmap;
        HeightmapDrawable->_uniforms = _sharedUniforms;
        return HeightmapDrawable;
    }

   void HeightmapDrawableFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        const graphics::ScenePointer& scene,
        const graphics::CameraPointer& camera,
        graphics::HeightmapDrawable& drawable)
    {
        // It s time to create a descriptorSet that matches the expected pipeline descriptor set
        // then we will assign a uniform buffer in it
        graphics::DescriptorSetInit descriptorSetInit{
            _HeightmapPipeline->getDescriptorSetLayout()
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

        auto heightmap = &drawable;
        auto pipeline = this->_HeightmapPipeline;

        // And now a render callback where we describe the rendering sequence
        graphics::DrawObjectCallback drawCallback = [heightmap, descriptorSet, pipeline](const NodeID node, const graphics::CameraPointer& camera, const graphics::SwapchainPointer& swapchain, const graphics::DevicePointer& device, const graphics::BatchPointer& batch) {
            batch->setPipeline(pipeline);
            batch->setViewport(camera->getViewportRect());
            batch->setScissor(camera->getViewportRect());

            batch->bindDescriptorSet(descriptorSet);

            HeightmapObjectData odata{ node, heightmap->_heightmap.width,  heightmap->_heightmap.height, heightmap->_heightmap.spacing };
            batch->bindPushUniform(1, sizeof(HeightmapObjectData), (const uint8_t*)&odata);

            // A hieghtmap is drawan with triangle strip patch of (2 * (width + 1) + 1) * (height) verts
            batch->draw(((2 * (heightmap->_heightmap.width + 1) + 1) * heightmap->_heightmap.height) - 1, 0);
        };
        drawable._drawcall = drawCallback;
    }

} // !namespace graphics