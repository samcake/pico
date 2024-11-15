// HeightmapDraw.cpp
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
#include "HeightmapDraw.h"

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

#include "Transform_inc.h"
#include "Projection_inc.h"
#include "Camera_inc.h"
#include "SceneTransform_inc.h"

#include "Heightmap_vert.h"
#include "Heightmap_frag.h"
#include "Ocean_comp.h"

//using namespace view3d;
namespace graphics
{

    HeightmapDrawFactory::HeightmapDrawFactory(const graphics::DevicePointer& device) :
        _sharedUniforms(std::make_shared<HeightmapDrawUniforms>()) {

        allocateGPUShared(device);
    }
    HeightmapDrawFactory::~HeightmapDrawFactory() {

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

    void HeightmapDrawFactory::allocateGPUShared(const graphics::DevicePointer& device) {

        // Let's describe the pipeline Descriptors layout
        graphics::RootDescriptorLayoutInit rootLayoutInit{ 
            { // Push uniforms layout
                { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::VERTEX, 1, sizeof(HeightmapObjectData) >> 2}
            },
            {
                // ViewPass descriptorSet Layout
                Viewport::viewPassLayout,
                { // Descriptor set Layouts
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 1, 1}
                }
            }
        };
        auto rootDescriptorLayout = device->createRootDescriptorLayout(rootLayoutInit);

        // And a Pipeline

        // test: create shader
        graphics::ShaderIncludeLib include = {
            Transform_inc::getMapEntry(),
            Projection_inc::getMapEntry(),
            Camera_inc::getMapEntry(),
            SceneTransform_inc::getMapEntry(),
        };
        graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "main", Heightmap_vert::getSource, Heightmap_vert::getSourceFilename(), include };
        graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main", Heightmap_frag::getSource, Heightmap_frag::getSourceFilename(), include };
        graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

        graphics::ProgramInit programInit{ vertexShader, pixelShader };
        graphics::ShaderPointer programShader = device->createProgram(programInit);

        graphics::GraphicsPipelineStateInit pipelineInit{
                    programShader,
                    rootDescriptorLayout,
                    StreamLayout(),
                    graphics::PrimitiveTopology::TRIANGLE_STRIP,
                    RasterizerState(),
                    {true}, // enable depth
                    BlendState()
        };
      //  pipelineInit.rasterizer.fillMode = graphics::FillMode::LINE;
        _HeightmapPipeline = device->createGraphicsPipelineState(pipelineInit);


        {
            // Let's describe the Compute pipeline Descriptors layout
            graphics::RootDescriptorLayoutInit computeDescriptorLayoutInit{
                { // Push uniforms layout
                    { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::COMPUTE, 1, sizeof(HeightmapObjectData) >> 2}
                },
                {{ // Descriptor set Layouts
                    { graphics::DescriptorType::RW_RESOURCE_BUFFER, graphics::ShaderStage::COMPUTE, 0, 1},
                }}
            };
            auto compRootDescriptorLayout = device->createRootDescriptorLayout(computeDescriptorLayoutInit);

            graphics::ShaderInit compShaderInit{ graphics::ShaderType::COMPUTE, "main", Ocean_comp::getSource, Ocean_comp::getSourceFilename() };
            graphics::ShaderPointer compShader = device->createShader(compShaderInit);

            graphics::ComputePipelineStateInit computePipelineInit{
                compShader,
                compRootDescriptorLayout
            };

            _computePipeline = device->createComputePipelineState(computePipelineInit);
        }
    }

    graphics::HeightmapDraw HeightmapDrawFactory::createHeightmap(const graphics::DevicePointer& device, const Heightmap& heightmap) {
        HeightmapDraw heightmapDraw;
        heightmapDraw._heightmap = heightmap;
        heightmapDraw._uniforms = _sharedUniforms;

        auto numHeights = heightmap.getMapNumElements();

        graphics::BufferInit hbresourceBufferInit{};
        hbresourceBufferInit.usage = (graphics::ResourceUsage::RESOURCE_BUFFER) | (heightmap.heights.empty() ? graphics::ResourceUsage::RW_RESOURCE_BUFFER : 0);
        hbresourceBufferInit.hostVisible = (heightmap.heights.size() > 0);
        hbresourceBufferInit.bufferSize = numHeights * sizeof(float);
        hbresourceBufferInit.firstElement = 0;
        hbresourceBufferInit.numElements = numHeights;
        hbresourceBufferInit.structStride = sizeof(float);

        heightmapDraw._heightBuffer = device->createBuffer(hbresourceBufferInit);

        if ( heightmap.heights.size() > 0) {
             memcpy(heightmapDraw._heightBuffer->_cpuMappedAddress, heightmap.heights.data(), hbresourceBufferInit.bufferSize);
        }

        allocateDrawcallObject(device, heightmapDraw);

        return heightmapDraw;
    }

   void HeightmapDrawFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        graphics::HeightmapDraw& draw)
    {
       bool doCompute = draw._heightmap.heights.empty();

       graphics::DescriptorSetInit compDescriptorSetInit{
           _computePipeline->getRootDescriptorLayout(),
           0
       };
       auto compDescriptorSet = device->createDescriptorSet(compDescriptorSetInit);

       if (doCompute) {
           graphics::DescriptorObjects compute_descriptorObjects = {{
                { graphics::DescriptorType::RW_RESOURCE_BUFFER, draw.getHeightBuffer() }
           }};
           device->updateDescriptorSet(compDescriptorSet, compute_descriptorObjects);
       }

        // Create DescriptorSet #1, #0 is ViewPassDS
        graphics::DescriptorSetInit descriptorSetInit{
            _HeightmapPipeline->getRootDescriptorLayout(),
            1
        };
        auto descriptorSet = device->createDescriptorSet(descriptorSetInit);

        graphics::DescriptorObjects descriptorObjects = {
           { graphics::DescriptorType::RESOURCE_BUFFER, draw.getHeightBuffer() }
        };
        device->updateDescriptorSet(descriptorSet, descriptorObjects);
        auto compute_pipeline = this->_computePipeline;
        auto pipeline = this->_HeightmapPipeline;

        // And now a render callback where we describe the rendering sequence
        draw._drawcall = [draw, doCompute, compDescriptorSet, compute_pipeline, descriptorSet, pipeline](const NodeID node, RenderArgs& args) {
  
            static uint32_t frameNum = 0;
            frameNum++;
            HeightmapObjectData odata{ frameNum, draw._heightmap.map_width,  draw._heightmap.map_height, draw._heightmap.map_spacing,
                                       draw._heightmap.mesh_resolutionX,  draw._heightmap.mesh_resolutionY, draw._heightmap.mesh_spacing };

            if (doCompute) {
                args.batch->bindPipeline(compute_pipeline);
                args.batch->bindDescriptorSet(graphics::PipelineType::COMPUTE, compDescriptorSet);
                args.batch->bindPushUniform(graphics::PipelineType::COMPUTE, 0, sizeof(HeightmapObjectData), (const uint8_t*)&odata);
            
            //   args.batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::SHADER_RESOURCE, graphics::ResourceState::UNORDERED_ACCESS, heightmap->getHeightBuffer());
                args.batch->dispatch(draw._heightmap.map_width / 32, draw._heightmap.map_height / 32);
                args.batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::UNORDERED_ACCESS, graphics::ResourceState::SHADER_RESOURCE, draw.getHeightBuffer());
            }

            odata.nodeID = node;
            args.batch->bindPipeline(pipeline);
            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet);
            args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(HeightmapObjectData), (const uint8_t*)&odata);

            // A heightmap is drawn with triangle strips patch of (2 * (width + 1) + 1) * (height) verts
            args.batch->draw(draw._heightmap.getMeshNumIndices(), 0);
        };
    }

} // !namespace graphics