// TriangleSoupDrawable.cpp
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
#include "TriangleSoupDrawable.h"

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

#include <document/TriangleSoup.h>

#include "TriangleSoup_vert.h"
#include "TriangleSoup_frag.h"

//using namespace view3d;
namespace graphics
{

    TriangleSoupDrawable::TriangleSoupDrawable() {

    }
    TriangleSoupDrawable::~TriangleSoupDrawable() {

    }
       
    DrawcallObjectPointer TriangleSoupDrawable::allocateDocumentDrawcallObject(const graphics::DevicePointer& device, const graphics::CameraPointer& camera, const document::TriangleSoupPointer& triangleSoup)
    {
        if (!triangleSoup) {
            return nullptr;
        }

        // Step 1, create a Mesh from the triangle soup data

        // Declare the vertex format == TriangleSoup::Point
        //pico::AttribArray<3> attribs{ {{ pico::AttribSemantic::A, pico::AttribFormat::VEC3, 0 }, { pico::AttribSemantic::B, pico::AttribFormat::VEC3, 0 }, {pico::AttribSemantic::C, pico::AttribFormat::CVEC4, 0 }} };
        AttribArray<2> attribs{ {{ graphics::AttribSemantic::A, graphics::AttribFormat::VEC3, 0 }, {graphics::AttribSemantic::C, graphics::AttribFormat::CVEC4, 0 }} };
        AttribBufferViewArray<1> bufferViews{ {{0, (uint32_t)triangleSoup->_points.size() }} };
        auto vertexFormat = graphics::StreamLayout::build<2,1>(attribs, bufferViews);

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

        // Custom data uniforms
        struct ObjectData {
            core::mat4x3 transform;
            uint32_t numVertices{ 0 };
            uint32_t numIndices{ 0 };
            uint32_t stride{ 0 };
            float B;
        };


        // Let's describe the pipeline Descriptors layout
        graphics::DescriptorLayouts descriptorLayouts{
            { graphics::DescriptorType::UNIFORM_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::VERTEX, 1, sizeof(ObjectData) >> 2},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 1, 1},
        };

        graphics::DescriptorSetLayoutInit descriptorSetLayoutInit{ descriptorLayouts };
        auto descriptorSetLayout = device->createDescriptorSetLayout(descriptorSetLayoutInit);

        // And a Pipeline

        // Load shaders (as stored in the resources)
        auto shader_vertex_src = TriangleSoup_vert::getSource();
        auto shader_pixel_src = TriangleSoup_frag::getSource();

        // test: create shader
        graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "main", "", shader_vertex_src };
        graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main", "", shader_pixel_src };
        graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

        graphics::ProgramInit programInit{ vertexShader, pixelShader };
        graphics::ShaderPointer programShader = device->createProgram(programInit);

        graphics::PipelineStateInit pipelineInit{
                    programShader,
                    StreamLayout(),
                    graphics::PrimitiveTopology::TRIANGLE,
                    descriptorSetLayout,
                    true // enable depth
        };
        graphics::PipelineStatePointer pipeline = device->createPipelineState(pipelineInit);

        // It s time to create a descriptorSet that matches the expected pipeline descriptor set
        // then we will assign a uniform buffer in it
        graphics::DescriptorSetInit descriptorSetInit{
            descriptorSetLayout
        };
        auto descriptorSet = device->createDescriptorSet(descriptorSetInit);

        // Assign the Camera UBO just created as the resource of the descriptorSet
        // auto descriptorObjects = descriptorSet->buildDescriptorObjects();
        graphics::DescriptorObject uboDescriptorObject;
        uboDescriptorObject._uniformBuffers.push_back(camera->getGPUBuffer());
        graphics::DescriptorObject vb_rboDescriptorObject;
        vb_rboDescriptorObject._buffers.push_back(vbresourceBuffer);
        graphics::DescriptorObject ib_rboDescriptorObject;
        ib_rboDescriptorObject._buffers.push_back(ibresourceBuffer);
        graphics::DescriptorObjects descriptorObjects = {
            uboDescriptorObject, vb_rboDescriptorObject, ib_rboDescriptorObject
        };
        device->updateDescriptorSet(descriptorSet, descriptorObjects);

        // And now a render callback where we describe the rendering sequence
        graphics::DrawObjectCallback drawCallback = [pipeline, descriptorSet, numVertices, numIndices, vertexStride](const core::mat4x3& transform, const graphics::CameraPointer& camera, const graphics::SwapchainPointer& swapchain, const graphics::DevicePointer& device, const graphics::BatchPointer& batch) {
            batch->setPipeline(pipeline);
            batch->setViewport(camera->getViewportRect());
            batch->setScissor(camera->getViewportRect());

            batch->bindDescriptorSet(descriptorSet);

            ObjectData odata{ transform, numVertices, numIndices, vertexStride };
            batch->bindPushUniform(1, sizeof(ObjectData), (const uint8_t*)&odata);

            batch->draw(numIndices, 0);
        };
        auto drawcall = new graphics::DrawcallObject(drawCallback);
        drawcall->_bounds = mesh->_bounds;
        drawcall->_transform = triangleSoup->_transform;
        _drawcall = graphics::DrawcallObjectPointer(drawcall);

        return _drawcall;
    }

    graphics::DrawcallObjectPointer TriangleSoupDrawable::getDrawable() const {
        return _drawcall;
    }

} // !namespace graphics