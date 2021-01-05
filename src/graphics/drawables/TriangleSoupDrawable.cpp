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

    TriangleSoupDrawableFactory::TriangleSoupDrawableFactory() :
        _sharedUniforms(std::make_shared<TriangleSoupDrawableUniforms>()) {

    }
    TriangleSoupDrawableFactory::~TriangleSoupDrawableFactory() {

    }

    // Custom data uniforms
    struct TSObjectData {
        core::ivec4 instance { 0 };
        uint32_t numVertices{ 0 };
        uint32_t numIndices{ 0 };
        uint32_t stride{ 0 };
        float triangleScale { 0.1f };
    };

    void TriangleSoupDrawableFactory::allocateGPUShared(const graphics::DevicePointer& device) {

        // Let's describe the pipeline Descriptors layout
        graphics::DescriptorLayouts descriptorLayouts{
            { graphics::DescriptorType::UNIFORM_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::VERTEX, 1, sizeof(TSObjectData) >> 2},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 1, 1},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 2, 1},

        };

        graphics::DescriptorSetLayoutInit descriptorSetLayoutInit{ descriptorLayouts };
        auto descriptorSetLayout = device->createDescriptorSetLayout(descriptorSetLayoutInit);

        // And a Pipeline

        // Load shaders (as stored in the resources)
        auto shader_vertex_src = TriangleSoup_vert::getSource();
        auto shader_pixel_src = TriangleSoup_frag::getSource();

        // test: create shader
        graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "main", "", shader_vertex_src, TriangleSoup_vert::getSourceFilename() };
        graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main", "", shader_pixel_src, TriangleSoup_frag::getSourceFilename() };
        graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

        graphics::ProgramInit programInit{ vertexShader, pixelShader };
        graphics::ShaderPointer programShader = device->createProgram(programInit);

        graphics::GraphicsPipelineStateInit pipelineInit{
                    programShader,
                    StreamLayout(),
                    graphics::PrimitiveTopology::TRIANGLE,
                    descriptorSetLayout,
                    RasterizerState(),
                    true, // enable depth
                    BlendState()
        };
        _pipeline = device->createGraphicsPipelineState(pipelineInit);
    }

    graphics::TriangleSoupDrawable* TriangleSoupDrawableFactory::createTriangleSoupDrawable(const graphics::DevicePointer& device, const document::TriangleSoupPointer& triangleSoup) {
        if (!triangleSoup) {
            return nullptr;
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


        auto triangleSoupDrawable = new TriangleSoupDrawable();
        triangleSoupDrawable->_vertexBuffer = vbresourceBuffer;
        triangleSoupDrawable->_indexBuffer = ibresourceBuffer;
        triangleSoupDrawable->_bounds = mesh->_bounds;

        // Create the triangle soup drawable using the shared uniforms of the factory
        triangleSoupDrawable->_uniforms = _sharedUniforms;

        return triangleSoupDrawable;
    }

    void TriangleSoupDrawableFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        const graphics::ScenePointer& scene,
        const graphics::CameraPointer& camera,
        graphics::TriangleSoupDrawable& triangleSoup)
    {
        // It s time to create a descriptorSet that matches the expected pipeline descriptor set
        // then we will assign a uniform buffer in it
        graphics::DescriptorSetInit descriptorSetInit{
            _pipeline->getDescriptorSetLayout()
        };
        auto descriptorSet = device->createDescriptorSet(descriptorSetInit);

        // Assign the Camera UBO just created as the resource of the descriptorSet
        // auto descriptorObjects = descriptorSet->buildDescriptorObjects();
        graphics::DescriptorObject camera_uboDescriptorObject;
        camera_uboDescriptorObject._uniformBuffers.push_back(camera->getGPUBuffer());
        graphics::DescriptorObject transform_rboDescriptorObject;
        transform_rboDescriptorObject._buffers.push_back(scene->_nodes._transforms_buffer);
        graphics::DescriptorObject vb_rboDescriptorObject;
        vb_rboDescriptorObject._buffers.push_back(triangleSoup.getVertexBuffer());
        graphics::DescriptorObject ib_rboDescriptorObject;
        ib_rboDescriptorObject._buffers.push_back(triangleSoup.getIndexBuffer());
        graphics::DescriptorObjects descriptorObjects = {
            camera_uboDescriptorObject, transform_rboDescriptorObject, vb_rboDescriptorObject, ib_rboDescriptorObject
        };
        device->updateDescriptorSet(descriptorSet, descriptorObjects);


        auto numVertices = triangleSoup.getVertexBuffer()->getNumElements();
        auto numIndices = triangleSoup.getIndexBuffer()->getNumElements();
        auto vertexStride = triangleSoup.getVertexBuffer()->_init.structStride;

        auto ptriangleSoup = &triangleSoup;
        auto pipeline = this->_pipeline;

        // And now a render callback where we describe the rendering sequence
        graphics::DrawObjectCallback drawCallback = [ptriangleSoup, descriptorSet, numVertices, numIndices, vertexStride, pipeline](
            const NodeID node,
            const graphics::CameraPointer& camera, 
            const graphics::SwapchainPointer& swapchain, 
            const graphics::DevicePointer& device, 
            const graphics::BatchPointer& batch) {
            batch->bindPipeline(pipeline);
            batch->setViewport(camera->getViewportRect());
            batch->setScissor(camera->getViewportRect());

            //       batch->bindVertexBuffers(1, &vertexBuffer);
            batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet);

            auto uniforms = ptriangleSoup->getUniforms();
            TSObjectData odata{ { (int32_t)node }, numVertices, numIndices, vertexStride, uniforms->triangleScale };
            batch->bindPushUniform(1, sizeof(TSObjectData), (const uint8_t*)&odata);

            batch->draw(numIndices, 0);
        };
        triangleSoup._drawcall = drawCallback;

    }


    TriangleSoupDrawable::TriangleSoupDrawable() {

    }
    TriangleSoupDrawable::~TriangleSoupDrawable() {

    }

} // !namespace graphics