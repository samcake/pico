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
#include "Ocean_comp.h"

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

        uint32_t mesh_width{ 0 };
        uint32_t mesh_height{ 0 };
        float mesh_spacing{ 0 };
    };

    void HeightmapDrawableFactory::allocateGPUShared(const graphics::DevicePointer& device) {

        // Let's describe the pipeline Descriptors layout
        graphics::DescriptorLayouts descriptorLayouts{
            { graphics::DescriptorType::UNIFORM_BUFFER, graphics::ShaderStage::ALL_GRAPHICS, 0, 1},
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::VERTEX, 1, sizeof(HeightmapObjectData) >> 2},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 1, 1},
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

        graphics::GraphicsPipelineStateInit pipelineInit{
                    programShader,
                    StreamLayout(),
                    graphics::PrimitiveTopology::TRIANGLE_STRIP,
                    descriptorSetLayout,
                    RasterizerState(),
                    true, // enable depth
                    BlendState()
        };
      //  pipelineInit.rasterizer.fillMode = graphics::FillMode::LINE;
        _HeightmapPipeline = device->createGraphicsPipelineState(pipelineInit);


        {
            // Let's describe the Compute pipeline Descriptors layout
            graphics::DescriptorLayouts computeDescriptorLayouts{
                { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::COMPUTE, 1, sizeof(HeightmapObjectData) >> 2},
                { graphics::DescriptorType::RW_RESOURCE_BUFFER, graphics::ShaderStage::COMPUTE, 0, 1},
            };
            graphics::DescriptorSetLayoutInit compDescriptorSetLayoutInit{ computeDescriptorLayouts };
            auto compDescriptorSetLayout = device->createDescriptorSetLayout(compDescriptorSetLayoutInit);

            graphics::ShaderInit compShaderInit{ graphics::ShaderType::COMPUTE, "main", "", Ocean_comp::getSource(), Ocean_comp::getSourceFilename() };
            graphics::ShaderPointer compShader = device->createShader(compShaderInit);

            graphics::ComputePipelineStateInit computePipelineInit{
                compShader,
                compDescriptorSetLayout
            };

            _computePipeline = device->createComputePipelineState(computePipelineInit);
        }
    }

    graphics::HeightmapDrawable* HeightmapDrawableFactory::createHeightmap(const graphics::DevicePointer& device, const Heightmap& heightmap) {
        auto HeightmapDrawable = new graphics::HeightmapDrawable();
        HeightmapDrawable->_heightmap = heightmap;
        HeightmapDrawable->_uniforms = _sharedUniforms;

        auto numHeights = heightmap.getMapNumElements();

        graphics::BufferInit hbresourceBufferInit{};
        hbresourceBufferInit.usage = (graphics::ResourceUsage::RESOURCE_BUFFER | graphics::ResourceUsage::RW_RESOURCE_BUFFER);
   //     hbresourceBufferInit.hostVisible = true;
        hbresourceBufferInit.bufferSize = numHeights * sizeof(float);
        hbresourceBufferInit.firstElement = 0;
        hbresourceBufferInit.numElements = numHeights;
        hbresourceBufferInit.structStride = sizeof(float);

        HeightmapDrawable->_heightBuffer = device->createBuffer(hbresourceBufferInit);
        // memcpy(HeightmapDrawable->_heightBuffer->_cpuMappedAddress, mesh->_vertexStream._buffers[0]->_data.data(), vbresourceBufferInit.bufferSize);

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
       graphics::DescriptorSetInit compDescriptorSetInit{
           _computePipeline->getDescriptorSetLayout()
       };
       auto compDescriptorSet = device->createDescriptorSet(compDescriptorSetInit);

       graphics::DescriptorObject compute_wrboDescriptorObject;
       compute_wrboDescriptorObject._buffers.push_back(drawable.getHeightBuffer());
       graphics::DescriptorObjects compute_descriptorObjects = {
            compute_wrboDescriptorObject
       };
       device->updateDescriptorSet(compDescriptorSet, compute_descriptorObjects);

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
        graphics::DescriptorObject heightmap_rboDescriptorObject;
        heightmap_rboDescriptorObject._buffers.push_back(drawable.getHeightBuffer());

        graphics::DescriptorObjects descriptorObjects = {
            camera_uboDescriptorObject,
            transform_rboDescriptorObject,
            heightmap_rboDescriptorObject
        };
        device->updateDescriptorSet(descriptorSet, descriptorObjects);

        auto heightmap = &drawable;
        auto compute_pipeline = this->_computePipeline;
        auto pipeline = this->_HeightmapPipeline;

        // And now a render callback where we describe the rendering sequence
        graphics::DrawObjectCallback drawCallback = [heightmap, compDescriptorSet, compute_pipeline, descriptorSet, pipeline](const NodeID node, const graphics::CameraPointer& camera, const graphics::SwapchainPointer& swapchain, const graphics::DevicePointer& device, const graphics::BatchPointer& batch) {
            batch->setViewport(camera->getViewportRect());
            batch->setScissor(camera->getViewportRect());
    
            static uint32_t frameNum = 0;
            frameNum++;
            HeightmapObjectData odata{ frameNum, heightmap->_heightmap.map_width,  heightmap->_heightmap.map_height, heightmap->_heightmap.map_spacing,
                                       heightmap->_heightmap.mesh_resolutionX,  heightmap->_heightmap.mesh_resolutionY, heightmap->_heightmap.mesh_spacing };

            batch->bindPipeline(compute_pipeline);
            batch->bindDescriptorSet(graphics::PipelineType::COMPUTE, compDescriptorSet);
            batch->bindPushUniform(graphics::PipelineType::COMPUTE, 0, sizeof(HeightmapObjectData), (const uint8_t*)&odata);
            
        //   batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::SHADER_RESOURCE, graphics::ResourceState::UNORDERED_ACCESS, heightmap->getHeightBuffer());
            batch->dispatch(heightmap->_heightmap.map_width / 32, heightmap->_heightmap.map_height / 32);
            batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::UNORDERED_ACCESS, graphics::ResourceState::SHADER_RESOURCE, heightmap->getHeightBuffer());

            odata.nodeID = node;
            batch->bindPipeline(pipeline);
            batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet);
            batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 1, sizeof(HeightmapObjectData), (const uint8_t*)&odata);

            // A heightmap is drawn with triangle strips patch of (2 * (width + 1) + 1) * (height) verts
            batch->draw(heightmap->_heightmap.getMeshNumIndices(), 0);
        };
        drawable._drawcall = drawCallback;
    }

} // !namespace graphics