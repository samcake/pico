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

        // Let's describe the pipeline Descriptors layout
        graphics::RootDescriptorLayoutInit rootLayoutInit{
            {
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::VERTEX, 1, sizeof(SkyDrawableData) >> 2}
            },
            {
                // ViewPass descriptorSet Layout
                Viewport::viewPassLayout
            }
         };
        auto rootDescriptorLayout = device->createRootDescriptorLayout(rootLayoutInit);

        // Shaders & Pipeline
        graphics::ShaderIncludeLib include = { 
            Transform_inc::getMapEntry(),
            Projection_inc::getMapEntry(),
            Camera_inc::getMapEntry(),
            SceneTransform_inc::getMapEntry(),
            Sky_inc::getMapEntry(),
            Color_inc::getMapEntry(),
        };
        graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "main", SkyDrawable_vert::getSource, SkyDrawable_vert::getSourceFilename(), include };
        graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main", SkyDrawable_frag::getSource, SkyDrawable_frag::getSourceFilename(), include };
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

        _sharedUniforms->_sky->allocateGPUData(device);
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
        auto pipeline = this->_skyPipeline;

        // And now a render callback where we describe the rendering sequence
        graphics::DrawObjectCallback drawCallback = [prim_, pipeline](const NodeID node, RenderArgs& args) {
            if (args.timer) {
                args.batch->bindPipeline(pipeline);
                args.batch->setViewport(args.camera->getViewportRect());
                args.batch->setScissor(args.camera->getViewportRect());

                args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);

                auto pushdata = evalPushDataFromUnifors((* (prim_->getUniforms()) ));
                args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(SkyDrawableData), (const uint8_t*)&pushdata);

                // A quad is drawn with one triangle 3 verts
                args.batch->draw(3 * args.timer->getNumSamples(), 0);
            }
        };
        prim._drawcall = drawCallback;
    }

} // !namespace graphics