// SkyDrawable.cpp
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
#include "SkyDrawable.h"

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

#include "Transform_inc.h"
#include "Projection_inc.h"
#include "Camera_inc.h"
#include "SceneTransform_inc.h"
#include "Sky_inc.h"
#include "Color_inc.h"

#include "SkyDrawable_vert.h"
#include "SkyDrawable_frag.h"

using float2 = core::vec2;
using float3 = core::vec3;
using float4 = core::vec4;

//using namespace view3d;
namespace graphics
{



    SkyDrawableFactory::SkyDrawableFactory() :
        _sharedUniforms(std::make_shared<SkyDrawableUniforms>()) {
        _sharedUniforms->_sky = std::make_shared<Sky>();

    }
    SkyDrawableFactory::~SkyDrawableFactory() {

    }

    // Custom data uniforms
    struct SkyDrawableData {
        float3 sunDir;
        float spare;
    };

    SkyDrawableData evalPushDataFromUnifors(const SkyDrawableUniforms& uniforms) {
        return { uniforms._sky->getSunDir(), 0 };
    }

    void SkyDrawableFactory::allocateGPUShared(const graphics::DevicePointer& device) {

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
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::VERTEX, 1, sizeof(SkyDrawableData) >> 2}
            },
            {
                // ViewPass descriptorSet Layout
                Viewport::viewPassLayout,
                {
                { graphics::DescriptorType::RESOURCE_TEXTURE, graphics::ShaderStage::ALL_GRAPHICS, 0, 1},
                { graphics::DescriptorType::RESOURCE_TEXTURE, graphics::ShaderStage::ALL_GRAPHICS, 1, 1},
                }
            },
            {
            { graphics::DescriptorType::SAMPLER, graphics::ShaderStage::ALL_GRAPHICS, 0, 2},
            }
         };
        auto rootDescriptorLayout = device->createRootDescriptorLayout(rootLayoutInit);

        // Shaders & Pipeline
        {

            graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "main", SkyDrawable_vert::getSource, SkyDrawable_vert::getSourceFilename(), include };
            graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

            graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main_draw", SkyDrawable_frag::getSource, SkyDrawable_frag::getSourceFilename(), include };
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
               graphics::ShaderInit skymap_compShaderInit{ graphics::ShaderType::COMPUTE, "main_makeSkymap", SkyDrawable_frag::getSource, SkyDrawable_frag::getSourceFilename(), include };
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
                { graphics::DescriptorType::RW_RESOURCE_TEXTURE, graphics::ShaderStage::COMPUTE, 0, 1}, // render target!
                { graphics::DescriptorType::RESOURCE_TEXTURE, graphics::ShaderStage::ALL_GRAPHICS, 0, 1},
                }
            },
            {
            { graphics::DescriptorType::SAMPLER, graphics::ShaderStage::ALL_GRAPHICS, 0, 2},
            }
        };
        auto diffuse_skymap_descriptorLayout = device->createRootDescriptorLayout(diffuse_compute_descriptorLayoutInit);


        {
            graphics::ShaderInit diffuse_skymap_compShaderInit{ graphics::ShaderType::COMPUTE, "main_makeDiffuseSkymap", SkyDrawable_frag::getSource, SkyDrawable_frag::getSourceFilename(), include };
            graphics::ShaderPointer diffuse_skymap_compShader = device->createShader(diffuse_skymap_compShaderInit);

            // Let's describe the Compute pipeline Descriptors layout
            graphics::ComputePipelineStateInit diffuse_skymap_pipelineInit{
                diffuse_skymap_compShader,
                diffuse_skymap_descriptorLayout
            };

            _diffuseSkymapPipeline = device->createComputePipelineState(diffuse_skymap_pipelineInit);
        }

    }

    graphics::SkyDrawable* SkyDrawableFactory::createDrawable(const graphics::DevicePointer& device) {
        auto primitiveDrawable = new SkyDrawable();
        primitiveDrawable->_uniforms = _sharedUniforms;
        return primitiveDrawable;
    }

   void SkyDrawableFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        const graphics::ScenePointer& scene,
        graphics::SkyDrawable& prim)
    {
        auto prim_ = &prim;
        auto drawPipeline = this->_skyPipeline;
        auto skymapPipeline = this->_skymapPipeline;
        auto diffusePipeline = this->_diffuseSkymapPipeline;

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
            { graphics::DescriptorType::RESOURCE_TEXTURE, prim.getUniforms()->_sky->getDiffuseSkymap() },
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
            diffusePipeline->getRootDescriptorLayout(),
            1, true
        };
        auto diffuse_descriptorSet = device->createDescriptorSet(diffuse_descriptorSetInit);

        graphics::DescriptorObjects diffuse_descriptorObjects = {
            { graphics::DescriptorType::RW_RESOURCE_TEXTURE, prim.getUniforms()->_sky->getDiffuseSkymap() },
            { graphics::DescriptorType::RESOURCE_TEXTURE, prim.getUniforms()->_sky->getSkymap() },
            { samplerP },
            { samplerL }
        };
        device->updateDescriptorSet(diffuse_descriptorSet, diffuse_descriptorObjects);




        // And now a render callback where we describe the rendering sequence
        graphics::DrawObjectCallback drawCallback = [prim_, drawPipeline, skymapPipeline, diffusePipeline, draw_descriptorSet, skymap_descriptorSet, diffuse_descriptorSet](const NodeID node, RenderArgs& args) {
            auto uniforms = prim_->getUniforms();
            if (uniforms->_sky->needSkymapUpdate()) {
                const int NUM_COMPUTE_GROUP_THREADS = 4;

                auto skymap = uniforms->_sky->getSkymap();
                args.batch->bindPipeline(skymapPipeline);
                args.batch->bindDescriptorSet(graphics::PipelineType::COMPUTE, args.viewPassDescriptorSet);
                args.batch->bindDescriptorSet(graphics::PipelineType::COMPUTE, skymap_descriptorSet);
                args.batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::SHADER_RESOURCE, graphics::ResourceState::UNORDERED_ACCESS, skymap);
                args.batch->dispatch(skymap->width() / NUM_COMPUTE_GROUP_THREADS, skymap->height() / NUM_COMPUTE_GROUP_THREADS);
                args.batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::UNORDERED_ACCESS, graphics::ResourceState::SHADER_RESOURCE, skymap);

                auto diffuse_skymap = uniforms->_sky->getDiffuseSkymap();
                args.batch->bindPipeline(diffusePipeline);
                args.batch->bindDescriptorSet(graphics::PipelineType::COMPUTE, args.viewPassDescriptorSet);
                args.batch->bindDescriptorSet(graphics::PipelineType::COMPUTE, diffuse_descriptorSet);
                args.batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::SHADER_RESOURCE, graphics::ResourceState::UNORDERED_ACCESS, diffuse_skymap);
                args.batch->dispatch(diffuse_skymap->width() / NUM_COMPUTE_GROUP_THREADS, diffuse_skymap->height() / NUM_COMPUTE_GROUP_THREADS);
                args.batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::UNORDERED_ACCESS, graphics::ResourceState::SHADER_RESOURCE, diffuse_skymap);
                    
                uniforms->_sky->resetNeedSkymapUpdate();
            }

            args.batch->bindPipeline(drawPipeline);
            args.batch->setViewport(args.camera->getViewportRect());
            args.batch->setScissor(args.camera->getViewportRect());

            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, draw_descriptorSet);

            auto pushdata = evalPushDataFromUnifors((* (uniforms) ));
            args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(SkyDrawableData), (const uint8_t*)&pushdata);

            // A quad is drawn with one triangle 3 verts
            args.batch->draw(3 * args.timer->getNumSamples(), 0);
        };
        prim._drawcall = drawCallback;
    }

} // !namespace graphics