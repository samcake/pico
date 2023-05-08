// SkyDraw.cpp
//
// Sam Gateau - October 2021
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
#include "SkyDraw.h"

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
#include "Sky_inc.h"
#include "Color_inc.h"

#include "SkyDraw_vert.h"
#include "SkyDraw_frag.h"
#include "SkyDraw_comp.h"

using float2 = core::vec2;
using float3 = core::vec3;
using float4 = core::vec4;

//using namespace view3d;
namespace graphics
{



    SkyDrawFactory::SkyDrawFactory(const graphics::DevicePointer& device) :
        _sharedUniforms(std::make_shared<SkyDrawUniforms>()) {
        _sharedUniforms->_sky = std::make_shared<Sky>();

        allocateGPUShared(device);
    }
    SkyDrawFactory::~SkyDrawFactory() {

    }

    // Custom data uniforms
    struct SkyDrawData {
        float3 sunDir;
        float spare;
    };

    SkyDrawData evalPushDataFromUnifors(const SkyDrawUniforms& uniforms) {
        return { uniforms._sky->getSunDir(), 0 };
    }

    void SkyDrawFactory::allocateGPUShared(const graphics::DevicePointer& device) {

        _sharedUniforms->_sky->allocateGPUData(device);


        graphics::ShaderIncludeLib include = {
            Transform_inc::getMapEntry(),
            Projection_inc::getMapEntry(),
            Camera_inc::getMapEntry(),
            SceneTransform_inc::getMapEntry(),
            Sky_inc::getMapEntry(),
            Color_inc::getMapEntry(),
        };
            
        // Let's describe the pipeline Descriptors layout
        graphics::RootDescriptorLayoutInit rootLayoutInit{
            {
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::VERTEX, 1, sizeof(SkyDrawData) >> 2}
            },
            {
                // ViewPass descriptorSet Layout
                Viewport::viewPassLayout,
                {
                { graphics::DescriptorType::RESOURCE_TEXTURE, graphics::ShaderStage::ALL_GRAPHICS, 0, 1},
                }
            },
            {
            { graphics::DescriptorType::SAMPLER, graphics::ShaderStage::ALL_GRAPHICS, 0, 2},
            }
         };
        auto rootDescriptorLayout = device->createRootDescriptorLayout(rootLayoutInit);

        // Shaders & Pipeline
        {

            graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "main", SkyDraw_vert::getSource, SkyDraw_vert::getSourceFilename(), include };
            graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

            graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main_draw", SkyDraw_frag::getSource, SkyDraw_frag::getSourceFilename(), include };
            graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

            graphics::ProgramInit programInit{ vertexShader, pixelShader };
            graphics::ShaderPointer programShader = device->createProgram(programInit);

            graphics::GraphicsPipelineStateInit pipelineInit{
                        programShader,
                        rootDescriptorLayout,
                        StreamLayout(),
                        graphics::PrimitiveTopology::TRIANGLE,
                        RasterizerState(),
                        DepthStencilState(true), // enable depth
                        BlendState()
            };
            _skyPipeline = device->createGraphicsPipelineState(pipelineInit);

        }

        // Let's describe the pipeline Descriptors layout for compute pass
        graphics::RootDescriptorLayoutInit compute_descriptorLayoutInit{
            {
            },
            {
                // ViewPass descriptorSet Layout
                Viewport::viewPassLayout,
                {
                { graphics::DescriptorType::RW_RESOURCE_TEXTURE, graphics::ShaderStage::COMPUTE, 0, 1}, // render target!
                }
            }
        };
        auto skymap_descriptorLayout = device->createRootDescriptorLayout(compute_descriptorLayoutInit);


        {
               graphics::ShaderInit skymap_compShaderInit{ graphics::ShaderType::COMPUTE, "main_makeSkymap", SkyDraw_comp::getSource, SkyDraw_comp::getSourceFilename(), include };
               graphics::ShaderPointer skymap_compShader = device->createShader(skymap_compShaderInit);

               // Let's describe the Compute pipeline Descriptors layout
               graphics::ComputePipelineStateInit skymap_pipelineInit{
                   skymap_compShader,
                   skymap_descriptorLayout
               };

               _skymapPipeline = device->createComputePipelineState(skymap_pipelineInit);
        }

        // Let's describe the pipeline Descriptors layout for diffuse compute pass
        graphics::RootDescriptorLayoutInit diffuse_compute_descriptorLayoutInit{
            {
            },
            {
                // ViewPass descriptorSet Layout
                Viewport::viewPassLayout,
                {
                { graphics::DescriptorType::RW_RESOURCE_BUFFER, graphics::ShaderStage::COMPUTE, 1, 1}, // render buffer!
                { graphics::DescriptorType::RESOURCE_TEXTURE, graphics::ShaderStage::ALL_GRAPHICS, 0, 1},
                }
            },
            {
            { graphics::DescriptorType::SAMPLER, graphics::ShaderStage::ALL_GRAPHICS, 0, 2},
            }
        };
        auto diffuse_skymap_descriptorLayout = device->createRootDescriptorLayout(diffuse_compute_descriptorLayoutInit);


        {
            graphics::ShaderInit diffuse_skymap_compShaderInit{ graphics::ShaderType::COMPUTE, "main_makeDiffuseSkymap_first", SkyDraw_comp::getSource, SkyDraw_comp::getSourceFilename(), include };
            graphics::ShaderPointer diffuse_skymap_compShader = device->createShader(diffuse_skymap_compShaderInit);

            // Let's describe the Compute pipeline Descriptors layout
            graphics::ComputePipelineStateInit diffuse_skymap_pipelineInit{
                diffuse_skymap_compShader,
                diffuse_skymap_descriptorLayout
            };

            _diffuseSkymapPipeline[0] = device->createComputePipelineState(diffuse_skymap_pipelineInit);

            graphics::ShaderInit diffuse_skymap_next_compShaderInit{ graphics::ShaderType::COMPUTE, "main_makeDiffuseSkymap_next", SkyDraw_comp::getSource, SkyDraw_comp::getSourceFilename(), include };
            graphics::ShaderPointer diffuse_skymap_next_compShader = device->createShader(diffuse_skymap_next_compShaderInit);

            // Let's describe the Compute pipeline Descriptors layout
            graphics::ComputePipelineStateInit diffuse_skymap_next_pipelineInit{
                diffuse_skymap_next_compShader,
                diffuse_skymap_descriptorLayout
            };
            _diffuseSkymapPipeline[1] = device->createComputePipelineState(diffuse_skymap_next_pipelineInit);
        }

    }

    graphics::SkyDraw SkyDrawFactory::createDraw(const graphics::DevicePointer& device) {
        SkyDraw primitiveDraw;
        primitiveDraw._uniforms = _sharedUniforms;

        allocateDrawcallObject(device, primitiveDraw);

        return primitiveDraw;
    }

   void SkyDrawFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        graphics::SkyDraw& prim)
    {
        auto prim_ = prim;
        auto drawPipeline = this->_skyPipeline;
        auto skymapPipeline = this->_skymapPipeline;
        auto diffusePipelineFirst = this->_diffuseSkymapPipeline[0];
        auto diffusePipelineNext = this->_diffuseSkymapPipeline[1];

        // It s time to create a descriptorSet that matches the expected pipeline descriptor set
        // then we will assign a uniform buffer in it

        graphics::DescriptorSetInit draw_descriptorSetInit{
            drawPipeline->getRootDescriptorLayout(),
            1, true
        };
        auto draw_descriptorSet = device->createDescriptorSet(draw_descriptorSetInit);

        graphics::SamplerInit samplerInit{};
        samplerInit._filter = graphics::Filter::MIN_MAG_MIP_POINT;
        auto samplerP = device->createSampler(samplerInit);

        samplerInit._filter = graphics::Filter::MIN_MAG_LINEAR_MIP_POINT;
        auto samplerL = device->createSampler(samplerInit);

        graphics::DescriptorObjects draw_descriptorObjects = {
            { graphics::DescriptorType::RESOURCE_TEXTURE, prim.getUniforms()->_sky->getSkymap() },
            { samplerP },
            { samplerL }
        };
        device->updateDescriptorSet(draw_descriptorSet, draw_descriptorObjects);

        graphics::DescriptorSetInit skymap_descriptorSetInit{
            skymapPipeline->getRootDescriptorLayout(),
            1, true
        };
        auto skymap_descriptorSet = device->createDescriptorSet(skymap_descriptorSetInit);

        graphics::DescriptorObjects skymap_descriptorObjects = {
            { graphics::DescriptorType::RW_RESOURCE_TEXTURE, prim.getUniforms()->_sky->getSkymap() }
        };
        device->updateDescriptorSet(skymap_descriptorSet, skymap_descriptorObjects);


        graphics::DescriptorSetInit diffuse_descriptorSetInit{
            diffusePipelineFirst->getRootDescriptorLayout(),
            1, true
        };
        auto diffuse_descriptorSet = device->createDescriptorSet(diffuse_descriptorSetInit);

        const int THREAD_GROUP_SIDE = 8;
        int32_t irradianceRes = prim.getUniforms()->_sky->getSkymap()->width();

        uint32_t numPixels = (irradianceRes * irradianceRes);
        uint32_t numBlocks = numPixels / (THREAD_GROUP_SIDE * THREAD_GROUP_SIDE);
        uint32_t sizeSh = 4 * 32 * 9;
        graphics::BufferInit bufInit;
        bufInit.bufferSize = numBlocks * sizeSh;
        bufInit.numElements = 9 * numBlocks;
        bufInit.structStride = 4 * 32;
        bufInit.usage = RW_RESOURCE_BUFFER | GENERIC_READ_BUFFER;
        auto diffuse_skybuf = device->createBuffer(bufInit);

        graphics::DescriptorObjects diffuse_descriptorObjects = {
            { graphics::DescriptorType::RW_RESOURCE_BUFFER, diffuse_skybuf },
            { graphics::DescriptorType::RESOURCE_TEXTURE, prim.getUniforms()->_sky->getSkymap() },
            { samplerP },
            { samplerL }
        };
        device->updateDescriptorSet(diffuse_descriptorSet, diffuse_descriptorObjects);

        // And now a render callback where we describe the rendering sequence
        prim._drawcall = [THREAD_GROUP_SIDE, prim_, drawPipeline, skymapPipeline, diffusePipelineFirst, diffusePipelineNext, draw_descriptorSet, skymap_descriptorSet, diffuse_descriptorSet, diffuse_skybuf](const NodeID node, RenderArgs& args) {
            auto& batch = args.batch;
            auto uniforms = prim_.getUniforms();
            if (uniforms->_sky->needSkymapUpdate()) {

                batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::VERTEX_AND_CONSTANT_BUFFER, graphics::ResourceState::COPY_DEST, uniforms->_sky->getGPUBuffer());
                batch->uploadBuffer(uniforms->_sky->getGPUBuffer());
                batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::COPY_DEST, graphics::ResourceState::VERTEX_AND_CONSTANT_BUFFER, uniforms->_sky->getGPUBuffer());

                auto skymap = uniforms->_sky->getSkymap();
                batch->bindPipeline(skymapPipeline);
                batch->bindDescriptorSet(graphics::PipelineType::COMPUTE, args.viewPassDescriptorSet);
                batch->bindDescriptorSet(graphics::PipelineType::COMPUTE, skymap_descriptorSet);
                batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::SHADER_RESOURCE, graphics::ResourceState::UNORDERED_ACCESS, skymap);
                batch->dispatch(skymap->width() / THREAD_GROUP_SIDE, skymap->height() / THREAD_GROUP_SIDE);
                batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::UNORDERED_ACCESS, graphics::ResourceState::SHADER_RESOURCE, skymap);

                int32_t irradianceRes = uniforms->_sky->getSimDim().z;
                core::ivec2 group_size(irradianceRes / THREAD_GROUP_SIDE, irradianceRes / THREAD_GROUP_SIDE);
                batch->bindPipeline(diffusePipelineFirst);
                batch->bindDescriptorSet(graphics::PipelineType::COMPUTE, args.viewPassDescriptorSet);
                batch->bindDescriptorSet(graphics::PipelineType::COMPUTE, diffuse_descriptorSet);
                batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::GENERIC_READ_BUFFER, graphics::ResourceState::UNORDERED_ACCESS, diffuse_skybuf);
                batch->dispatch(group_size.x * group_size.y);


                batch->bindPipeline(diffusePipelineNext);
                batch->bindDescriptorSet(graphics::PipelineType::COMPUTE, diffuse_descriptorSet);

                group_size.x >>= 1;
                group_size.y >>= 1;
                while (!(group_size.x == 0 && group_size.y == 0))
                {
                    batch->resourceBarrierRW(graphics::ResourceBarrierFlag::NONE, diffuse_skybuf);
                
                    batch->dispatch(group_size.x * group_size.y);
                    group_size.x >>= 1;
                    group_size.y >>= 1;
                }
                batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::UNORDERED_ACCESS, graphics::ResourceState::GENERIC_READ_BUFFER, diffuse_skybuf);
                
                // Now copy the sh values computed back oin the sky const buffer
                auto skyUniformBuffer = uniforms->_sky->getGPUBuffer();
                batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::VERTEX_AND_CONSTANT_BUFFER, graphics::ResourceState::COPY_DEST, skyUniformBuffer);
                batch->copyBufferRegion(skyUniformBuffer, uniforms->_sky->getIrradianceSHOffsetInGPUBuffer(), diffuse_skybuf, 0, sizeof(SphericalHarmonics));
                batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::COPY_DEST, graphics::ResourceState::VERTEX_AND_CONSTANT_BUFFER, skyUniformBuffer);

                uniforms->_sky->resetNeedSkymapUpdate();
            }

            batch->bindPipeline(drawPipeline);

            batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
            batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, draw_descriptorSet);

            auto pushdata = evalPushDataFromUnifors((* (uniforms) ));
            batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(SkyDrawData), (const uint8_t*)&pushdata);

            // A quad is drawn with one triangle 3 verts
            batch->draw(3 * args.timer->getNumSamples(), 0);
        };
    }

} // !namespace graphics