// DashboardDraw.cpp
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
#include "DashboardDraw.h"

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
#include "Viewport_inc.h"

#include "Dashboard_vert.h"
#include "Dashboard_frag.h"

//using namespace view3d;
namespace graphics
{

    DashboardDrawFactory::DashboardDrawFactory(const graphics::DevicePointer& device) :
        _sharedUniforms(std::make_shared<DashboardDrawUniforms>()) {
        allocateGPUShared(device);
    }
    DashboardDrawFactory::~DashboardDrawFactory() {

    }

    // Custom data uniforms
    struct PrimitiveObjectData {
        int32_t nodeID{0};
        int32_t numVertices{ 0 };
        float numIndices{ 0 };
        float stride{ 0 };
    };

    void DashboardDrawFactory::allocateGPUShared(const graphics::DevicePointer& device) {

        // Let's describe the pipeline Descriptors layout
        graphics::RootDescriptorLayoutInit rootLayoutInit{
            {
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::VERTEX, 1, sizeof(PrimitiveObjectData) >> 2}
            },
            {
                // ViewPass descriptorSet Layout
                Viewport::viewPassLayout
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
           Viewport_inc::getMapEntry(),
        };
        graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "main", Dashboard_vert::getSource, Dashboard_vert::getSourceFilename(), include };
        graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main", Dashboard_frag::getSource, Dashboard_frag::getSourceFilename(), include };
        graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

        graphics::ProgramInit programInit{ vertexShader, pixelShader };
        graphics::ShaderPointer programShader = device->createProgram(programInit);

        graphics::GraphicsPipelineStateInit pipelineInit{
                    programShader,
                    rootDescriptorLayout,
                    StreamLayout(),
                    graphics::PrimitiveTopology::TRIANGLE,
                    RasterizerState(),
                    {true}, // enable depth
                    BlendState()
        };
        _primitivePipeline = device->createGraphicsPipelineState(pipelineInit);
    }

    graphics::DashboardDraw DashboardDrawFactory::createDraw(
        const graphics::DevicePointer& device)
    {
        DashboardDraw primitiveDraw;
        primitiveDraw._uniforms = _sharedUniforms;
        allocateDrawcallObject(device, primitiveDraw);

        return primitiveDraw;
    }

   void DashboardDrawFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        graphics::DashboardDraw& prim)
    {
        auto pipeline = this->_primitivePipeline;

        // And now a render callback where we describe the rendering sequence
        prim._drawcall = [pipeline](const NodeID node, RenderArgs& args) {
            if (args.timer) {
                args.batch->bindPipeline(pipeline);

                args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
                PrimitiveObjectData odata{ args.timer->getCurrentSampleIndex(), args.timer->getNumSamples(), 1.0f, 1.0f };
                args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(PrimitiveObjectData), (const uint8_t*)&odata);

                // A quad is drawn with one triangle 3 verts
                args.batch->draw(3 * args.timer->getNumSamples(), 0);
            }
        };
    }


   graphics::Item DashboardDraw_createSceneWidgets(const ScenePointer& scene, const DevicePointer& gpuDevice) {
       // A gizmo draw factory
       auto dashboardDrawFactory = std::make_shared<DashboardDrawFactory>(gpuDevice);

       // a dashboard
       auto dashboardDraw = scene->createDraw(dashboardDrawFactory->createDraw(gpuDevice));
       auto dashboard = scene->createItem({ .node = INVALID_NODE_ID, .draw = dashboardDraw.id() });
       dashboard.setVisible(false);

       return dashboard;
   }

} // !namespace graphics