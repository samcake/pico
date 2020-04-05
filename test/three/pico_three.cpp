// pico_three.cpp 
//
// Sam Gateau - January 2020
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

#include <functional>
#include <chrono>

#include <pico/pico.h>

#include <pico/window/Window.h>

#include <pico/gpu/Device.h>
#include <pico/gpu/Resource.h>
#include <pico/gpu/Shader.h>
#include <pico/gpu/Pipeline.h>
#include <pico/gpu/Batch.h>
#include <pico/gpu/Swapchain.h>

#include <pico/render/Renderer.h>
#include <pico/render/Mesh.h>

#include <pico/content/PointCloud.h>


#include <vector>

//--------------------------------------------------------------------------------------

pico::PipelineStatePointer createPipelineState(const pico::DevicePointer& device, pico::StreamLayout vertexLayout) {

    pico::ShaderInit vertexShaderInit{ pico::ShaderType::VERTEX, "mainVertex", "./vertex.hlsl" };
    pico::ShaderPointer vertexShader = device->createShader(vertexShaderInit);


    pico::ShaderInit pixelShaderInit{ pico::ShaderType::PIXEL, "mainPixel", "./pixel.hlsl" };
    pico::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

    pico::ProgramInit programInit{ vertexShader, pixelShader };
    pico::ShaderPointer programShader = device->createProgram(programInit);



    pico::PipelineStateInit pipelineInit{ programShader,  vertexLayout, pico::PrimitiveTopology::POINT };
    pico::PipelineStatePointer pipeline = device->createPipelineState(pipelineInit);

    return pipeline;
}
//--------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Create the pico api
    pico::ApiInit pico_init{};
    auto result = pico::api::create(pico_init);

    if (!result) {
        std::clog << "Pico api failed to create ?" << std::endl;
        return 1;
    }

    // First a device, aka the gpu api used by pico
    pico::DeviceInit deviceInit{};
    auto gpuDevice = pico::Device::createDevice(deviceInit);


    // Content creation

    core::vec4 viewportRect{ 0.0f, 0.0f, 1280.0f, 720.f };


    // Some content, why not a pointcloud ?
    auto pointCloud = document::PointCloud::createFromPLY("../asset/20191211-brain.ply");


    // Step 1, create a Mesh from the point cloud data

    // Declare the vertex format == PointCloud::Point
    pico::Attribs<2> attribs{ {{ pico::AttribSemantic::A, pico::AttribFormat::VEC3, 0 }, {pico::AttribSemantic::C, pico::AttribFormat::CVEC4, 0 }} };
    pico::AttribBufferViews<1> bufferViews{ {0} };
    auto vertexFormat = pico::StreamLayout::build(attribs, bufferViews);

    // Create the Mesh for real
    pico::MeshPointer mesh = pico::Mesh::createFromPointArray(vertexFormat, (uint32_t)pointCloud->_points.size(), (const uint8_t*)pointCloud->_points.data());

    // Let's allocate buffer to hold the point cloud mesh
    pico::BufferInit vertexBufferInit{};
    vertexBufferInit.usage = pico::ResourceUsage::VERTEX_BUFFER;
    vertexBufferInit.hostVisible = true;
    vertexBufferInit.bufferSize = mesh->_vertexBuffers._buffers[0]->getSize();
    vertexBufferInit.vertexStride = mesh->_vertexBuffers._streamLayout.evalBufferViewByteStride(0);

    auto vertexBuffer = gpuDevice->createBuffer(vertexBufferInit);
    memcpy(vertexBuffer->_cpuMappedAddress, mesh->_vertexBuffers._buffers[0]->_data.data(), vertexBufferInit.bufferSize);

    auto numVertices = mesh->getNumVertices();

  /*  std::vector<uint32_t> indexData = {
        0, 2, 1,
        0, 2, 3
    };
    pico::BufferInit indexBufferInit{};
    indexBufferInit.usage = pico::ResourceState::INDEX_BUFFER;
    indexBufferInit.hostVisible = true;
    indexBufferInit.bufferSize = sizeof(uint32_t) * indexData.size();
    auto indexBuffer = gpuDevice->createBuffer(indexBufferInit);
    memcpy(indexBuffer->_cpuMappedAddress, indexData.data(), vertexBufferInit.bufferSize);
*/

    // And a Pipeline
    pico::PipelineStatePointer pipeline = createPipelineState(gpuDevice, mesh->_vertexBuffers._streamLayout);

    // Renderer creation
    // And now a render callback where we describe the rendering sequence
    pico::RenderCallback renderCallback = [&](const pico::CameraPointer& camera, const pico::SwapchainPointer& swapchain, const pico::DevicePointer& device, const pico::BatchPointer& batch) {
        static float time = 0.0f;
        time += 1.0f / 60.0f;
        float intPart;
        time = modf(time, &intPart);

        auto currentIndex = swapchain->currentIndex();

        batch->begin(currentIndex);

        batch->resourceBarrierTransition(
            pico::ResourceBarrierFlag::NONE,
            pico::ResourceState::PRESENT,
            pico::ResourceState::RENDER_TARGET,
            swapchain, currentIndex, -1);

        core::vec4 clearColor(core::colorRGBfromHSV(core::vec3(0.1f, 0.1f, 0.3f)), 1.f);
        batch->clear(swapchain, currentIndex, clearColor);

        batch->beginPass(swapchain, currentIndex);

        batch->setPipeline(pipeline);

        batch->bindVertexBuffers(1, &vertexBuffer);

        batch->setViewport(viewportRect);
        batch->setScissor(viewportRect);

        batch->draw(numVertices, 0);

        batch->endPass();

        batch->resourceBarrierTransition(
            pico::ResourceBarrierFlag::NONE,
            pico::ResourceState::RENDER_TARGET,
            pico::ResourceState::PRESENT,
            swapchain, currentIndex, -1);

        batch->end();

        device->executeBatch(batch);

        device->presentSwapchain(swapchain);
    };

    // Next, a renderer built on this device and render callback
    auto renderer = std::make_shared<pico::Renderer>(gpuDevice, renderCallback);

    // Presentation creation

    // We need a window where to present, let s use the pico::Window for convenience
    // This could be any window, we just need the os handle to create the swapchain next.
    auto windowHandler = new pico::WindowHandlerDelegate();
    pico::WindowInit windowInit { windowHandler };
    auto window = pico::Window::createWindow(windowInit);

    pico::SwapchainInit swapchainInit { (uint32_t) viewportRect.z, (uint32_t) viewportRect.w, (HWND) window->nativeWindow() };
    auto swapchain = gpuDevice->createSwapchain(swapchainInit);

    //Now that we have created all the elements, 
    // We configure the windowHandler onPaint delegate of the window to do real rendering!
    windowHandler->_onPaintDelegate = ([swapchain, renderer](const pico::PaintEvent& e) {
        // Measuring framerate
        static uint64_t frameCounter = 0;
        static double elapsedSeconds = 0.0;
        static std::chrono::high_resolution_clock clock;
        static auto t0 = clock.now();

        frameCounter++;
        auto t1 = clock.now();
        auto deltaTime = t1 - t0;
        t0 = t1;

        elapsedSeconds += deltaTime.count() * 1e-9;
        if (elapsedSeconds > 1.0) {
            char buffer[500];
            auto fps = frameCounter / elapsedSeconds;
            sprintf_s(buffer, 500, "FPS: %f\n", fps);
            OutputDebugString(buffer);
            frameCounter = 0;
            elapsedSeconds = 0.0;
        }


        // Render!
        renderer->render(nullptr, swapchain);
    });

    // Render Loop 
    bool keepOnGoing = true;
    while (keepOnGoing) {
        keepOnGoing = window->messagePump();
    }

    pico::api::destroy();
    std::clog << "Pico api destroyed" << std::endl;

     return 0;
}
