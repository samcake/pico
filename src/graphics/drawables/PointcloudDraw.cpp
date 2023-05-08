// PointCloud_Draw.cpp
//
// Sam Gateau - March 2020
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
#include "PointCloudDraw.h"

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

#include <document/pointcloud.h>


#include "Transform_inc.h"
#include "Projection_inc.h"
#include "Camera_inc.h"
#include "SceneTransform_inc.h"

#include "PointCloud_vert.h"
#include "PointCloud_frag.h"

//using namespace view3d;
namespace graphics
{
    PointCloudDrawFactory::PointCloudDrawFactory(const graphics::DevicePointer& device) :
        _sharedUniforms( std::make_shared<PointCloudDrawUniforms>()) {
        allocateGPUShared(device);
    }
    PointCloudDrawFactory::~PointCloudDrawFactory() {

    }

    // Custom data uniforms
    struct PCObjectData {
        core::ivec4 instance;
        float spriteSize{ 1.0f };
        float perspectiveSprite{ 1.0f };
        float perspectiveDepth{ 1.0f };
        float showPerspectiveDepthPlane{ 0.0f };
    };

    void PointCloudDrawFactory::allocateGPUShared(const graphics::DevicePointer& device) {

        graphics::RootDescriptorLayoutInit descriptorLayoutInit{
            {
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::VERTEX, 1, sizeof(PCObjectData) >> 2}
            },
            {
                // ViewPass descriptorSet Layout
                Viewport::viewPassLayout,
                {
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 1, 1},
                }
            }
        };
        auto rootDescriptorLayout = device->createRootDescriptorLayout(descriptorLayoutInit);

        // And a Pipeline

        // test: create shader
        graphics::ShaderIncludeLib include = {
            Transform_inc::getMapEntry(),
            Projection_inc::getMapEntry(),
            Camera_inc::getMapEntry(),
            SceneTransform_inc::getMapEntry(),
        };
        graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "main", PointCloud_vert::getSource, PointCloud_vert::getSourceFilename(), include };
        graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main", PointCloud_frag::getSource, PointCloud_frag::getSourceFilename(), include };
        graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

        graphics::ProgramInit programInit{ vertexShader, pixelShader };
        graphics::ShaderPointer programShader = device->createProgram(programInit);

        /*     graphics::PipelineStateInit pipelineInit0{
                  programShader,
                  mesh->_vertexBuffers._streamLayout,
                  graphics::PrimitiveTopology::POINT,
                  descriptorSetLayout,
                  true // enable depth
              };
              graphics::PipelineStatePointer pipeline0 = device->createPipelineState(pipelineInit0);
      */
        graphics::GraphicsPipelineStateInit pipelineInit{
                    programShader,
                    rootDescriptorLayout,
                    StreamLayout(),
                    graphics::PrimitiveTopology::TRIANGLE,
                    RasterizerState(),
                    DepthStencilState(true), // enable depth
                    { graphics::BlendFunction(true,
                        graphics::BlendArg::SRC_ALPHA, graphics::BlendOp::ADD, graphics::BlendArg::INV_SRC_ALPHA,
                        graphics::BlendArg::ONE, graphics::BlendOp::ADD, graphics::BlendArg::ZERO) }
        };
        _pipeline = device->createGraphicsPipelineState(pipelineInit);

    }

    graphics::PointCloudDraw PointCloudDrawFactory::createPointCloudDraw(const graphics::DevicePointer& device, const document::PointCloudPointer& pointCloud) {
        if (!pointCloud) {
            return PointCloudDraw();
        }

        // Step 1, create a Mesh from the point cloud data

        // Declare the vertex format == PointCloud::Point
        //graphics::Attribs<3> attribs{ {{ graphics::AttribSemantic::A, graphics::AttribFormat::VEC3, 0 }, { graphics::AttribSemantic::B, graphics::AttribFormat::VEC3, 0 }, {graphics::AttribSemantic::C, graphics::AttribFormat::CVEC4, 0 }} };
        graphics::AttribArray<2> attribs{ {{ graphics::AttribSemantic::A, graphics::AttribFormat::VEC3, 0 }, {graphics::AttribSemantic::C, graphics::AttribFormat::CVEC4, 0 }} };
        graphics::AttribBufferViewArray<1> bufferViews{ {0} };
        auto vertexFormat = graphics::StreamLayout::build(attribs, bufferViews);

        // Create the Mesh for real
        graphics::MeshPointer mesh = graphics::Mesh::createFromPointArray(vertexFormat, (uint32_t)pointCloud->_points.size(), (const uint8_t*)pointCloud->_points.data());

        // Let's allocate buffer to hold the point cloud mesh
        graphics::BufferInit vertexBufferInit{};
        vertexBufferInit.usage = graphics::ResourceUsage::VERTEX_BUFFER;
        vertexBufferInit.hostVisible = true;
        vertexBufferInit.bufferSize = mesh->_vertexStream._buffers[0]->getSize();
        vertexBufferInit.vertexStride = mesh->_vertexStream._streamLayout.evalBufferViewByteStride(0);

        auto vertexBuffer = device->createBuffer(vertexBufferInit);
        memcpy(vertexBuffer->_cpuMappedAddress, mesh->_vertexStream._buffers[0]->_data.data(), vertexBufferInit.bufferSize);

        auto numVertices = mesh->getNumVertices();

        graphics::BufferInit resourceBufferInit{};
        resourceBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        resourceBufferInit.hostVisible = true;
        resourceBufferInit.bufferSize = mesh->_vertexStream._buffers[0]->getSize();
        resourceBufferInit.firstElement = 0;
        resourceBufferInit.numElements = numVertices;
        resourceBufferInit.structStride = mesh->_vertexStream._streamLayout.evalBufferViewByteStride(0);

        auto resourceBuffer = device->createBuffer(resourceBufferInit);
        memcpy(resourceBuffer->_cpuMappedAddress, mesh->_vertexStream._buffers[0]->_data.data(), resourceBufferInit.bufferSize);

        PointCloudDraw pointCloudDraw;
        pointCloudDraw._vertexBuffer = resourceBuffer;
        pointCloudDraw._bounds = mesh->_bounds;
        pointCloudDraw._transform = pointCloud->_transform;
    
        // Create the point cloud draw using the shared uniforms of the factory
        pointCloudDraw._uniforms = _sharedUniforms;

        allocateDrawcallObject(device, pointCloudDraw);

        return pointCloudDraw;
    }

    void PointCloudDrawFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        graphics::PointCloudDraw& pointcloud)
    {
        graphics::DescriptorSetInit descriptorSetInit{
            _pipeline->getRootDescriptorLayout(),
            1
        };
        auto descriptorSet = device->createDescriptorSet(descriptorSetInit);

        graphics::DescriptorObjects descriptorObjects = {
           { graphics::DescriptorType::RESOURCE_BUFFER, pointcloud.getVertexBuffer() }
        };
        device->updateDescriptorSet(descriptorSet, descriptorObjects);

        auto numVertices = pointcloud.getVertexBuffer()->numElements();

        // And now a render callback where we describe the rendering sequence
        pointcloud._drawcall = [ppointcloud = pointcloud, descriptorSet, numVertices, pipeline = this->_pipeline](const NodeID node, RenderArgs& args) {
            args.batch->bindPipeline(pipeline);

     //       args.batch->bindVertexBuffers(1, &vertexBuffer);
            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet);

            auto uniforms = ppointcloud.getUniforms();
            PCObjectData odata { { (int32_t) node }, uniforms->spriteSize, uniforms->perspectiveSprite, uniforms->perspectiveDepth, uniforms->showPerspectiveDepthPlane };
            args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(PCObjectData), (const uint8_t*) &odata);

            args.batch->draw(3 * numVertices, 0);
        };
    }

} // !namespace graphics