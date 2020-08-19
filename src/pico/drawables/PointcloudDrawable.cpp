// PointCloud_Drawable.cpp
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
#include "PointCloudDrawable.h"

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

#include <content/pointcloud.h>

#include "PointCloud_vert.h"
#include "PointCloud_frag.h"

//using namespace view3d;
namespace pico
{
    PointCloudDrawableFactory::PointCloudDrawableFactory() :
        _sharedUniforms( std::make_shared<PointCloudDrawableUniforms>()) {

    }
    PointCloudDrawableFactory::~PointCloudDrawableFactory() {

    }

    // Custom data uniforms
    struct PCObjectData {
        core::mat4x3 transform;
        float spriteSize{ 1.0f };
        float perspectiveSprite{ 1.0f };
        float perspectiveDepth{ 1.0f };
        float showPerspectiveDepthPlane{ 0.0f };
    };

    void PointCloudDrawableFactory::allocateGPUShared(const pico::DevicePointer& device) {

        // Let's describe the pipeline Descriptors layout
        pico::DescriptorLayouts descriptorLayouts{
            { pico::DescriptorType::UNIFORM_BUFFER, pico::ShaderStage::VERTEX, 0, 1},
            { pico::DescriptorType::PUSH_UNIFORM, pico::ShaderStage::VERTEX, 1, sizeof(PCObjectData) >> 2},
            { pico::DescriptorType::RESOURCE_BUFFER, pico::ShaderStage::VERTEX, 0, 1},
        };

        pico::DescriptorSetLayoutInit descriptorSetLayoutInit{ descriptorLayouts };
        auto descriptorSetLayout = device->createDescriptorSetLayout(descriptorSetLayoutInit);

        // And a Pipeline

        // Load shaders (as stored in the resources)
        auto shader_vertex_src = PointCloud_vert::getSource();
        auto shader_pixel_src = PointCloud_frag::getSource();

        // test: create shader
        pico::ShaderInit vertexShaderInit{ pico::ShaderType::VERTEX, "main", "", shader_vertex_src };
        pico::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

        pico::ShaderInit pixelShaderInit{ pico::ShaderType::PIXEL, "main", "", shader_pixel_src };
        pico::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

        pico::ProgramInit programInit{ vertexShader, pixelShader };
        pico::ShaderPointer programShader = device->createProgram(programInit);

        /*     pico::PipelineStateInit pipelineInit0{
                  programShader,
                  mesh->_vertexBuffers._streamLayout,
                  pico::PrimitiveTopology::POINT,
                  descriptorSetLayout,
                  true // enable depth
              };
              pico::PipelineStatePointer pipeline0 = device->createPipelineState(pipelineInit0);
      */
        pico::PipelineStateInit pipelineInit{
                    programShader,
                    StreamLayout(),
                    pico::PrimitiveTopology::TRIANGLE,
                    descriptorSetLayout,
                    true // enable depth
        };
        _pipeline = device->createPipelineState(pipelineInit);

    }

    pico::PointCloudDrawable* PointCloudDrawableFactory::createPointCloudDrawable(const pico::DevicePointer& device, const document::PointCloudPointer& pointCloud) {
        if (!pointCloud) {
            return nullptr;
        }

        // Step 1, create a Mesh from the point cloud data

        // Declare the vertex format == PointCloud::Point
        //pico::Attribs<3> attribs{ {{ pico::AttribSemantic::A, pico::AttribFormat::VEC3, 0 }, { pico::AttribSemantic::B, pico::AttribFormat::VEC3, 0 }, {pico::AttribSemantic::C, pico::AttribFormat::CVEC4, 0 }} };
        pico::AttribArray<2> attribs{ {{ pico::AttribSemantic::A, pico::AttribFormat::VEC3, 0 }, {pico::AttribSemantic::C, pico::AttribFormat::CVEC4, 0 }} };
        pico::AttribBufferViewArray<1> bufferViews{ {0} };
        auto vertexFormat = pico::StreamLayout::build(attribs, bufferViews);

        // Create the Mesh for real
        pico::MeshPointer mesh = pico::Mesh::createFromPointArray(vertexFormat, (uint32_t)pointCloud->_points.size(), (const uint8_t*)pointCloud->_points.data());

        // Let's allocate buffer to hold the point cloud mesh
        pico::BufferInit vertexBufferInit{};
        vertexBufferInit.usage = pico::ResourceUsage::VERTEX_BUFFER;
        vertexBufferInit.hostVisible = true;
        vertexBufferInit.bufferSize = mesh->_vertexStream._buffers[0]->getSize();
        vertexBufferInit.vertexStride = mesh->_vertexStream._streamLayout.evalBufferViewByteStride(0);

        auto vertexBuffer = device->createBuffer(vertexBufferInit);
        memcpy(vertexBuffer->_cpuMappedAddress, mesh->_vertexStream._buffers[0]->_data.data(), vertexBufferInit.bufferSize);

        auto numVertices = mesh->getNumVertices();

        pico::BufferInit resourceBufferInit{};
        resourceBufferInit.usage = pico::ResourceUsage::RESOURCE_BUFFER;
        resourceBufferInit.hostVisible = true;
        resourceBufferInit.bufferSize = mesh->_vertexStream._buffers[0]->getSize();
        resourceBufferInit.firstElement = 0;
        resourceBufferInit.numElements = numVertices;
        resourceBufferInit.structStride = mesh->_vertexStream._streamLayout.evalBufferViewByteStride(0);

        auto resourceBuffer = device->createBuffer(resourceBufferInit);
        memcpy(resourceBuffer->_cpuMappedAddress, mesh->_vertexStream._buffers[0]->_data.data(), resourceBufferInit.bufferSize);

        auto pointCloudDrawable = new PointCloudDrawable();
        pointCloudDrawable->_vertexBuffer = resourceBuffer;
        pointCloudDrawable->_bounds = mesh->_bounds;
        pointCloudDrawable->_transform = pointCloud->_transform;
    
        // Create the point cloud drawable using the shared uniforms of the factory
        pointCloudDrawable->_uniforms = _sharedUniforms;

        return pointCloudDrawable;
    }

    pico::DrawcallObjectPointer PointCloudDrawableFactory::allocateDrawcallObject(const pico::DevicePointer& device,
         const pico::CameraPointer& camera, const pico::PointCloudDrawablePointer& pointcloud)
    {
        // It s time to create a descriptorSet that matches the expected pipeline descriptor set
        // then we will assign a uniform buffer in it
        pico::DescriptorSetInit descriptorSetInit{
            _pipeline->getDescriptorSetLayout()
        };
        auto descriptorSet = device->createDescriptorSet(descriptorSetInit);

        // Assign the Camera UBO just created as the resource of the descriptorSet
        // auto descriptorObjects = descriptorSet->buildDescriptorObjects();
        pico::DescriptorObject uboDescriptorObject;
        uboDescriptorObject._uniformBuffers.push_back(camera->getGPUBuffer());
        pico::DescriptorObject rboDescriptorObject;
        rboDescriptorObject._buffers.push_back(pointcloud->getVertexBuffer());
        pico::DescriptorObjects descriptorObjects = {
            uboDescriptorObject, rboDescriptorObject
        };
        device->updateDescriptorSet(descriptorSet, descriptorObjects);

        auto numVertices = pointcloud->getVertexBuffer()->getNumElements();

        // And now a render callback where we describe the rendering sequence
        pico::DrawObjectCallback drawCallback = [pointcloud, descriptorSet, numVertices, this](const core::mat4x3& transform, const pico::CameraPointer& camera, const pico::SwapchainPointer& swapchain, const pico::DevicePointer& device, const pico::BatchPointer& batch) {
            batch->setPipeline(this->_pipeline);
            batch->setViewport(camera->getViewportRect());
            batch->setScissor(camera->getViewportRect());

     //       batch->bindVertexBuffers(1, &vertexBuffer);
            batch->bindDescriptorSet(descriptorSet);

            auto uniforms = pointcloud->getUniforms();
            PCObjectData odata { transform, uniforms->spriteSize, uniforms->perspectiveSprite, uniforms->perspectiveDepth, uniforms->showPerspectiveDepthPlane };
            batch->bindPushUniform(1, sizeof(PCObjectData), (const uint8_t*) &odata);

            batch->draw(3 * numVertices, 0);
        };
        auto drawcall = new pico::DrawcallObject(drawCallback);
        drawcall->_bounds = pointcloud->_bounds;
        drawcall->_transform = pointcloud->_transform;
        pointcloud->_drawcall = pico::DrawcallObjectPointer(drawcall);

        return pointcloud->_drawcall;
    }


    PointCloudDrawable::PointCloudDrawable() {

    }
    PointCloudDrawable::~PointCloudDrawable() {

    }

    pico::DrawcallObjectPointer PointCloudDrawable::getDrawable() const {
        return _drawcall;
    }

} // !namespace pico