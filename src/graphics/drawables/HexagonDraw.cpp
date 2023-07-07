// HexagonDraw.cpp
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
#include "HexagonDraw.h"

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

#include "Color_inc.h"
#include "Hexagon_inc.h"

#include "Hexagon_vert.h"
#include "Hexagon_frag.h"

//using namespace view3d;
namespace graphics
{

    HexagonDrawFactory::HexagonDrawFactory(const graphics::DevicePointer& device) :
        _sharedUniforms(std::make_shared<HexagonDrawUniforms>()) {
        allocateGPUShared(device);
    }
    HexagonDrawFactory::~HexagonDrawFactory() {

    }

    // Custom data uniforms
    struct PrimitiveObjectData {
        uint32_t nodeID{0};
        float numVertices{ 0 };
        float numIndices{ 0 };
        float stride{ 0 };
    };

    void HexagonDrawFactory::allocateGPUShared(const graphics::DevicePointer& device) {

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

            Color_inc::getMapEntry(),
            Hexagon_inc::getMapEntry(),
        };

        graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "main_fsv", Hexagon_vert::getSource, Hexagon_vert::getSourceFilename(), include };
        graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main_hexamap", Hexagon_frag::getSource, Hexagon_frag::getSourceFilename(), include };
        graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

        graphics::ProgramInit programInit{ vertexShader, pixelShader };
        graphics::ShaderPointer programShader = device->createProgram(programInit);

        graphics::GraphicsPipelineStateInit pipelineInit{
                    programShader,
                    rootDescriptorLayout,
                    StreamLayout(),
                    graphics::PrimitiveTopology::TRIANGLE,
                    RasterizerState(),
                    { true }, // enable depth
                    BlendState()
        };
        _primitivePipeline = device->createGraphicsPipelineState(pipelineInit);
    }

    graphics::HexagonDraw HexagonDrawFactory::createDraw(const graphics::DevicePointer& device) {
        HexagonDraw primitiveDraw;
        primitiveDraw._uniforms = _sharedUniforms;

        allocateDrawcallObject(device, primitiveDraw);

        return primitiveDraw;
    }

   void HexagonDrawFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        graphics::HexagonDraw& prim)
    {
        auto prim_ = &prim;
        auto pipeline = this->_primitivePipeline;

        // And now a render callback where we describe the rendering sequence
       /* graphics::DrawObjectCallback drawCallback = [prim_, pipeline](const NodeID node, RenderArgs& args) {
            args.batch->bindPipeline(pipeline);
            args.batch->setViewport(args.camera->getViewportRect());
            args.batch->setScissor(args.camera->getViewportRect());

            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
            PrimitiveObjectData odata{ 0, 0, 1.0f, 1.0f };
            args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(PrimitiveObjectData), (const uint8_t*)&odata);


            // A hex is drawnumPentsn with 6 triangles 3 verts
            // A Pent is drawn with 5 triangles 3 verts
            int numPents = 12;
            int numFaces = 20;
            int numRings = 1;
            int numHexs = 1 + 6 * (numRings * (numRings + 1)) / 2;
            args.batch->draw(5 * 3 * numPents + 6 * 3 * numHexs * numFaces, 0);
        };*/


        // And now a render callback where we describe the rendering sequence
        prim._drawcall = [prim_, pipeline](const NodeID node, RenderArgs& args) {
            args.batch->bindPipeline(pipeline);
            args.batch->setViewport(args.camera->getViewportRect());
            args.batch->setScissor(args.camera->getViewportRect());

            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
            PrimitiveObjectData odata{ 0, 0, 1.0f, 1.0f };
            args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(PrimitiveObjectData), (const uint8_t*)&odata);


            // A hex is drawnumPentsn with 6 triangles 3 verts
           // int numHexs = 50;
           // int numIndicesPerHex = 6 * 3;
           // args.batch->draw(numHexs * numIndicesPerHex, 0);
           args.batch->draw(3, 0);

        };
    }

} // !namespace graphics