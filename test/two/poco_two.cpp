// poco_two.cpp 
//
// Sam Gateau - 2020/1/1
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

#include <poco/poco.h>

#include <poco/Window.h>

#include <poco/gpu/Device.h>
#include <poco/gpu/Resource.h>
#include <poco/gpu/Shader.h>
#include <poco/gpu/Pipeline.h>
#include <poco/gpu/Batch.h>
#include <poco/gpu/Swapchain.h>

#include <poco/render/Scene.h>
#include <poco/render/Renderer.h>
#include <poco/render/Viewport.h>

#include <vector>


class MyWindowHandler : public poco::WindowHandler {
public:
    std::function<void()> _onPaintDelegate;

    MyWindowHandler() {
    }

    void onPaint() override { 
        if (_onPaintDelegate) _onPaintDelegate();
    }

};
//--------------------------------------------------------------------------------------

poco::PipelineStatePointer createPipelineState(const poco::DevicePointer& device) {

    poco::ShaderInit vertexShaderInit{ poco::ShaderType::VERTEX, "mainVertex", "./vertex.hlsl" };
    poco::ShaderPointer vertexShader = device->createShader(vertexShaderInit);


    poco::ShaderInit pixelShaderInit{ poco::ShaderType::PIXEL, "mainPixel", "./pixel.hlsl" };
    poco::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

    poco::ProgramInit programInit { vertexShader, pixelShader };
    poco::ShaderPointer programShader = device->createProgram(programInit);



    poco::PipelineStateInit pipelineInit { programShader };
    poco::PipelineStatePointer pipeline = device->createPipelineState(pipelineInit);

    return pipeline;
}

//--------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Create the poco api
    poco::ApiInit poco_init{ hInstance };
    auto result = poco::api::create(poco_init);

    if (!result) {
        std::clog << "Poco api failed to create ?" << std::endl;
        return 1;
    }

    // Content creation

    // let's create a scene
    auto scene = std::make_shared<poco::Scene>();

    // A Camera added to the scene
    auto camera = std::make_shared<poco::Camera>(scene);

    // Renderer creation

    // First a device, aka the gpu api used by poco
    poco::DeviceInit deviceInit {};
    auto gpuDevice = poco::api::createDevice(deviceInit);

    // We need a Batch too, where to express the device commands
    poco::BatchInit batchInit {};
    auto batch = gpuDevice->createBatch(batchInit);


    // Let's allocate buffer
    // quad
    std::vector<float> vertexData = {
        -0.25f,  0.25f, 0.0f, 1.0f,
        -0.25f, -0.25f, 0.0f, 1.0f,
        0.25f, -0.25f, 0.0f, 1.0f,
        0.25f,  0.25f, 0.0f, 1.0f,
    };

    vertexData[4 * 0 + 0] += 0.5f;
    vertexData[4 * 1 + 0] += 0.5f;
    vertexData[4 * 2 + 0] += 0.5f;
    vertexData[4 * 3 + 0] += 0.5f;

    poco::BufferInit vertexBufferInit{};
    vertexBufferInit.usage = poco::ResourceState::VERTEX_AND_CONSTANT_BUFFER;
    vertexBufferInit.hostVisible = true;
    vertexBufferInit.bufferSize = sizeof(float) * vertexData.size();
   // vertexBufferInit.vertexStride = sizeof(float) * 4;
    auto vertexBuffer = gpuDevice->createBuffer(vertexBufferInit);
    memcpy(vertexBuffer->_cpuMappedAddress, vertexData.data(), vertexBufferInit.bufferSize);

    std::vector<uint32_t> indexData = {
        0, 2, 1,
        0, 2, 3
    };
    poco::BufferInit indexBufferInit{};
    indexBufferInit.usage = poco::ResourceState::INDEX_BUFFER;
    indexBufferInit.hostVisible = true;
    indexBufferInit.bufferSize = sizeof(uint32_t) * indexData.size();
    auto indexBuffer = gpuDevice->createBuffer(indexBufferInit);
    memcpy(indexBuffer->_cpuMappedAddress, indexData.data(), vertexBufferInit.bufferSize);

    // And a Pipeline
    poco::PipelineStatePointer pipeline = createPipelineState(gpuDevice);


    // And now a render callback where we describe the rendering sequence
    poco::RenderCallback renderCallback = [&](const poco::CameraPointer& camera, poco::SwapchainPointer& swapchain, poco::DevicePointer& device, poco::BatchPointer& batch) {
        poco::vec4 viewportRect { 0.0f, 0.0f, 640.0f, 480.f };

        auto currentIndex = swapchain->currentIndex();

        batch->begin(currentIndex);

        batch->resourceBarrierTransition(
            poco::ResourceBarrierFlag::NONE,
            poco::ResourceState::PRESENT,
            poco::ResourceState::RENDER_TARGET,
            swapchain, currentIndex, -1);

        static float time = 0.0f;
        time += 1.0f / 60.0f;
        float intPart;
        time = modf(time, &intPart);
       // poco::vec4 clearColor(colorRGBfromHSV(vec3(time, 0.5f, 1.f)), 1.f);
        poco::vec4 clearColor(poco::colorRGBfromHSV(poco::vec3(0.5f, 0.5f, 1.f)), 1.f);
        batch->clear(clearColor, swapchain, currentIndex);

        batch->beginPass(swapchain, currentIndex);

        batch->setPipeline(pipeline);

        batch->bindIndexBuffer(indexBuffer);
        batch->bindVertexBuffers(1, &vertexBuffer);

        batch->setViewport(viewportRect);
        batch->setScissor(viewportRect);

        batch->drawIndexed(6, 0);

        batch->endPass();

        batch->resourceBarrierTransition(
            poco::ResourceBarrierFlag::NONE,
            poco::ResourceState::RENDER_TARGET,
            poco::ResourceState::PRESENT,
            swapchain, currentIndex, -1);

        batch->end();

        device->executeBatch(batch);

        device->presentSwapchain(swapchain);
    };


    // Next, a renderer built on this device
    auto renderer = std::make_shared<poco::Renderer>(gpuDevice, renderCallback);


    // Presentation creation

    // We need a window where to present, let s use the poco::Window for convenience
    // This could be any window, we just need the os handle to create the swapchain next.
    auto windowHandler = new MyWindowHandler();
    poco::WindowInit windowInit { windowHandler };
    auto window =poco::api::createWindow(windowInit);

    poco::SwapchainInit swapchainInit { 640, 480, (HWND) window->nativeWindow() };
    auto swapchain = gpuDevice->createSwapchain(swapchainInit);

    // Finally, the viewport brings the Renderer, the Swapchain and the Camera in the Scene together to produce a render
    auto viewport = std::make_shared<poco::Viewport>(camera, renderer, swapchain);

    //Now that we have created all the elements, 
    // We configure the windowHandler onPaint delegate of the window to do real rendering!
    windowHandler->_onPaintDelegate = ([viewport]() {
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
        viewport->render();
    });

    // Render Loop 
    bool keepOnGoing = true;
    while (keepOnGoing) {
        keepOnGoing = window->messagePump();
    }

    poco::api::destroy();
    std::clog << "Poco api destroyed" << std::endl;

     return 0;
}
