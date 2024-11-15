// TriangleSoupDraw.cpp
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
#include "TriangleSoupDraw.h"

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

#include <document/TriangleSoup.h>

#include "Transform_inc.h"
#include "Projection_inc.h"
#include "Camera_inc.h"
#include "SceneTransform_inc.h"

#include "TriangleSoup_vert.h"
#include "TriangleSoup_frag.h"

//using namespace view3d;
namespace graphics
{

    TriangleSoupDrawFactory::TriangleSoupDrawFactory(const graphics::DevicePointer& device) :
        _sharedUniforms(std::make_shared<TriangleSoupDrawUniforms>()) {
            allocateGPUShared(device);
    }
    TriangleSoupDrawFactory::~TriangleSoupDrawFactory() {

    }

    // Custom data uniforms
    struct TSObjectData {
        core::ivec4 instance { 0 };
        uint32_t numVertices{ 0 };
        uint32_t numIndices{ 0 };
        uint32_t stride{ 0 };
        float triangleScale { 0.1f };
    };

    void TriangleSoupDrawFactory::allocateGPUShared(const graphics::DevicePointer& device) {

        // Let's describe the pipeline Descriptors layout
        graphics::RootDescriptorLayoutInit rootDescriptorLayoutInit{
        {
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::VERTEX, 1, sizeof(TSObjectData) >> 2}
        },
        {
            // ViewPass descriptorSet Layout
            Viewport::viewPassLayout,
            {
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 1, 1},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 2, 1},
            }
        }
        };
        auto rootDescriptorLayout = device->createRootDescriptorLayout(rootDescriptorLayoutInit);

        // And a Pipeline

        // test: create shader
        graphics::ShaderIncludeLib include = {
            Transform_inc::getMapEntry(),
            Projection_inc::getMapEntry(),
            Camera_inc::getMapEntry(),
            SceneTransform_inc::getMapEntry(),
        };
        graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "main", TriangleSoup_vert::getSource, TriangleSoup_vert::getSourceFilename(), include };
        graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main", TriangleSoup_frag::getSource, TriangleSoup_frag::getSourceFilename(), include };
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
        _pipeline = device->createGraphicsPipelineState(pipelineInit);
    }

    graphics::TriangleSoupDraw TriangleSoupDrawFactory::createTriangleSoupDraw(const graphics::DevicePointer& device, const document::TriangleSoupPointer& triangleSoup) {
        if (!triangleSoup) {
            return TriangleSoupDraw();
        }

        // Step 1, create a Mesh from the triangle soup data

        // Declare the vertex format == TriangleSoup::Point
        //pico::AttribArray<3> attribs{ {{ pico::AttribSemantic::A, pico::AttribFormat::VEC3, 0 }, { pico::AttribSemantic::B, pico::AttribFormat::VEC3, 0 }, {pico::AttribSemantic::C, pico::AttribFormat::CVEC4, 0 }} };
        AttribArray<2> attribs{ {{ graphics::AttribSemantic::A, graphics::AttribFormat::VEC3, 0 }, {graphics::AttribSemantic::C, graphics::AttribFormat::CVEC4, 0 }} };
        AttribBufferViewArray<1> bufferViews{ {{0, (uint32_t)triangleSoup->_points.size() }} };
        auto vertexFormat = graphics::StreamLayout::build<2, 1>(attribs, bufferViews);

        // Create the Mesh for real
        graphics::MeshPointer mesh = graphics::Mesh::createFromIndexedTriangleArray(vertexFormat, (uint32_t)triangleSoup->_points.size(), (const uint8_t*)triangleSoup->_points.data(),
            triangleSoup->_indices.size(), triangleSoup->_indices.data());

        // Let's allocate buffer to hold the triangle soup mesh
  /*      pico::BufferInit vertexBufferInit{};
        vertexBufferInit.usage = graphics::ResourceUsage::VERTEX_BUFFER;
        vertexBufferInit.hostVisible = true;
        vertexBufferInit.bufferSize = mesh->_vertexStream._buffers[0]->getSize();
        vertexBufferInit.vertexStride = mesh->_vertexStream._streamLayout.evalBufferViewByteStride(0);

        auto vertexBuffer = device->createBuffer(vertexBufferInit);
        memcpy(vertexBuffer->_cpuMappedAddress, mesh->_vertexStream._buffers[0]->_data.data(), vertexBufferInit.bufferSize);
    */
        auto numVertices = mesh->getNumVertices();
        auto vertexBufferSize = vertexFormat.streamAttribSize(0) * numVertices;
        uint32_t vertexStride = mesh->_vertexStream._streamLayout.evalBufferViewByteStride(0);

        graphics::BufferInit vbresourceBufferInit{};
        vbresourceBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        vbresourceBufferInit.hostVisible = true;
        vbresourceBufferInit.bufferSize = vertexBufferSize;
        vbresourceBufferInit.firstElement = 0;
        vbresourceBufferInit.numElements = numVertices;
        vbresourceBufferInit.structStride = vertexStride;

        auto vbresourceBuffer = device->createBuffer(vbresourceBufferInit);
        memcpy(vbresourceBuffer->_cpuMappedAddress, mesh->_vertexStream._buffers[0]->_data.data(), vbresourceBufferInit.bufferSize);


        auto numIndices = mesh->_indexStream.getNumElements();
        auto indexBufferSize = mesh->_indexStream._streamLayout.streamAttribSize(0) * numIndices;
        uint32_t indexStride = mesh->_indexStream._streamLayout.evalBufferViewByteStride(0);

        graphics::BufferInit ibresourceBufferInit{};
        ibresourceBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        ibresourceBufferInit.hostVisible = true;
        ibresourceBufferInit.bufferSize = indexBufferSize;
        ibresourceBufferInit.firstElement = 0;
        ibresourceBufferInit.numElements = numIndices;
        ibresourceBufferInit.structStride = indexStride;

        auto ibresourceBuffer = device->createBuffer(ibresourceBufferInit);
        memcpy(ibresourceBuffer->_cpuMappedAddress, mesh->_indexStream._buffers[0]->_data.data(), ibresourceBufferInit.bufferSize);


        TriangleSoupDraw triangleSoupDraw;
        triangleSoupDraw._vertexBuffer = vbresourceBuffer;
        triangleSoupDraw._indexBuffer = ibresourceBuffer;
        triangleSoupDraw._bounds = mesh->_bounds;

        // Create the triangle soup draw using the shared uniforms of the factory
        triangleSoupDraw._uniforms = _sharedUniforms;

        allocateDrawcallObject(device, triangleSoupDraw);

        return triangleSoupDraw;
    }

    void TriangleSoupDrawFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        graphics::TriangleSoupDraw& triangleSoup)
    {
        // It s time to create a descriptorSet that matches the expected pipeline descriptor set
        // then we will assign a uniform buffer in it
        graphics::DescriptorSetInit descriptorSetInit{
            _pipeline->getRootDescriptorLayout(),
            1
        };
        auto descriptorSet = device->createDescriptorSet(descriptorSetInit);

        // Assign the the resource of the descriptorSet
        graphics::DescriptorObjects descriptorObjects = {
            { graphics::DescriptorType::RESOURCE_BUFFER, triangleSoup.getVertexBuffer() },
            { graphics::DescriptorType::RESOURCE_BUFFER, triangleSoup.getIndexBuffer() }
        };
        device->updateDescriptorSet(descriptorSet, descriptorObjects);


        auto numVertices = triangleSoup.getVertexBuffer()->numElements();
        auto numIndices = triangleSoup.getIndexBuffer()->numElements();
        auto vertexStride = triangleSoup.getVertexBuffer()->_init.structStride;

        // And now a render callback where we describe the rendering sequence
        triangleSoup._drawcall = [ptriangleSoup = triangleSoup, descriptorSet, numVertices, numIndices, vertexStride, pipeline = this->_pipeline](
            const NodeID node,
            RenderArgs& args) {
            args.batch->bindPipeline(pipeline);

            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
            //       args.batch->bindVertexBuffers(1, &vertexBuffer);
            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet);

            auto uniforms = ptriangleSoup.getUniforms();
            TSObjectData odata{ { (int32_t)node }, numVertices, numIndices, vertexStride, uniforms->triangleScale };
            args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(TSObjectData), (const uint8_t*)&odata);

            args.batch->draw(numIndices, 0);
        };
    }

} // !namespace graphics