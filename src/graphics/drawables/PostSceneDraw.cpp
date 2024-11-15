// PostSceneDraw.cpp
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
#include "PostSceneDraw.h"

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

#include "PostScene_ray.h"

//using namespace view3d;
namespace graphics
{

    PostSceneDrawFactory::PostSceneDrawFactory(const graphics::DevicePointer& device) :
        _sharedUniforms(std::make_shared<PostSceneDrawUniforms>()) {
        allocateGPUShared(device);
    }
    PostSceneDrawFactory::~PostSceneDrawFactory() {

    }

    // Custom data uniforms
    struct PrimitiveObjectData {
        uint32_t nodeID{0};
        float numVertices{ 0 };
        float numIndices{ 0 };
        float stride{ 0 };
    };

    struct LViewport
    {
        float left;
        float top;
        float right;
        float bottom;
    };

    struct RayGenConstantBuffer
    {
        LViewport viewport;
        LViewport stencil;
    };

    void PostSceneDrawFactory::allocateGPUShared(const graphics::DevicePointer& device) {

        // Let's describe the pipeline Descriptors layout
        graphics::RootDescriptorLayoutInit globalRootLayoutInit{
            {
            },
            {{
                 { graphics::DescriptorType::RW_RESOURCE_TEXTURE, graphics::ShaderStage::RAYTRACING, 0, 1},
                 { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::RAYTRACING, 0, 1}
             }
            },
            {},
            PipelineType::RAYTRACING,
            false
         };
        auto globalRootDescriptorLayout = device->createRootDescriptorLayout(globalRootLayoutInit);

        graphics::RootDescriptorLayoutInit localRootLayoutInit{
            {
          //      { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::RAYTRACING, 0, sizeof(RayGenConstantBuffer) >> 2},
            },
            {{
                 { graphics::DescriptorType::RW_RESOURCE_TEXTURE, graphics::ShaderStage::RAYTRACING, 0, 1},
                 { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::RAYTRACING, 0, 1}
            }},
            {},
            PipelineType::RAYTRACING,
            true
        };
        auto localRootDescriptorLayout = device->createRootDescriptorLayout(localRootLayoutInit);

        // And a Pipeline

        // test: create shader
        graphics::ShaderInit raytracingShaderInit{ graphics::ShaderType::RAYTRACING, "main", PostScene_ray::getSource, PostScene_ray::getSourceFilename() };
        graphics::ShaderPointer raytracingShader = device->createShader(raytracingShaderInit);


        graphics::ProgramInit programInit{};
        programInit.type = PipelineType::RAYTRACING;
        programInit.shaderLib["RAYTRACING"] = raytracingShader;

        graphics::ShaderPointer programShader = device->createProgram(programInit);

        graphics::RaytracingPipelineStateInit pipelineInit{
                    programShader,
                    globalRootDescriptorLayout,
                    localRootDescriptorLayout
        };
        _primitivePipeline = device->createRaytracingPipelineState(pipelineInit);
    }

    graphics::PostSceneDraw PostSceneDrawFactory::createDraw(const graphics::DevicePointer& device, const graphics::GeometryPointer& geometry) {
        PostSceneDraw primitiveDraw;
        primitiveDraw._uniforms = _sharedUniforms;
        primitiveDraw._geometry = geometry;

        allocateDrawcallObject(device, primitiveDraw);

        return primitiveDraw;
    }

   void PostSceneDrawFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        graphics::PostSceneDraw& prim)
    {


       graphics::TextureInit texInit = {};
       texInit.width = 1024;
       texInit.height = 1024;
       texInit.usage = graphics::RW_RESOURCE_TEXTURE | graphics::RESOURCE_TEXTURE;
       prim._output = device->createTexture(texInit);

        auto prim_ = &prim;
        auto pipeline = this->_primitivePipeline;

        graphics::DescriptorSetInit descriptorSetInit{
            this->_primitivePipeline->getLocalRootDescriptorLayout(),
            0
        };
        auto descriptorSet = device->createDescriptorSet(descriptorSetInit);

        graphics::DescriptorObjects descriptorObjects = { {
             { graphics::DescriptorType::RW_RESOURCE_TEXTURE, prim._output },
             { graphics::DescriptorType::RESOURCE_BUFFER, prim._geometry->getTLAS()},
        } };
        device->updateDescriptorSet(descriptorSet, descriptorObjects);

        graphics::ShaderTableInit sti;
        sti.records.push_back({ device->getShaderEntry(this->_primitivePipeline, "MyRaygenShader"), descriptorSet });
        sti.records.push_back({ device->getShaderEntry(this->_primitivePipeline, "MyMissShader"), descriptorSet });
        sti.records.push_back({ device->getShaderEntry(this->_primitivePipeline, "HitGroup_Name"), descriptorSet });

        prim._shaderTable = device->createShaderTable(sti);
        auto shaderTable = prim._shaderTable;

        // And now a render callback where we describe the rendering sequence
        prim._drawcall = [prim_, pipeline, shaderTable](const NodeID node, RenderArgs& args) {

            graphics::Batch::DispatchRaysArgs rayArgs = {};
            rayArgs.shaderTable = shaderTable;
            
            rayArgs.generationShaderRecordStart = 0;
            rayArgs.generationShaderRecordSize = 64;
            
            rayArgs.missShaderRecordStart = 64;
            rayArgs.missShaderRecordSize = 64;
            rayArgs.missShaderRecordStride = 64;

            rayArgs.hitGroupShaderRecordStart = 128;
            rayArgs.hitGroupShaderRecordSize = 64;
            rayArgs.hitGroupShaderRecordStride = 64;

            rayArgs.width = 1024;
            rayArgs.height = 1024;

            args.batch->bindPipeline(pipeline);
            args.batch->dispatchRays(rayArgs);
        };
    }

} // !namespace graphics