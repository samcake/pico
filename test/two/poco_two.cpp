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
#include <poco/Scene.h>
#include <poco/Device.h>
#include <poco/Resource.h>
#include <poco/Pipeline.h>
#include <poco/Batch.h>
#include <poco/Window.h>
#include <poco/Swapchain.h>
#include <poco/Renderer.h>
#include <poco/Viewport.h>


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
    poco::BufferInit bufferInit {};
    auto buffer = gpuDevice->createBuffer(bufferInit);

    // And a Pipeline


    // And now a render callback where we describe the rendering sequence
    poco::RenderCallback renderCallback = [&](const poco::CameraPointer& camera, poco::SwapchainPointer& swapchain, poco::DevicePointer& device, poco::BatchPointer& batch) {
        auto currentIndex = swapchain->currentIndex();

        batch->begin(currentIndex);

        batch->resourceBarrierTransition(
            poco::Batch::BarrierFlag::NONE,
            poco::Batch::ResourceState::PRESENT,
            poco::Batch::ResourceState::RENDER_TARGET,
            swapchain, currentIndex, -1);

        static float time = 0.0f;
        time += 1.0f / 60.0f;
        float intPart;
        time = modf(time, &intPart);
       // poco::vec4 clearColor(colorRGBfromHSV(vec3(time, 0.5f, 1.f)), 1.f);
        poco::vec4 clearColor(poco::colorRGBfromHSV(poco::vec3(0.5f, 0.5f, 1.f)), 1.f);

/*
        auto backBuffer = m_pWindow->GetCurrentBackBuffer();
        auto rtv = m_pWindow->GetCurrentRenderTargetView();
        auto dsv = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();

        // Clear the render targets.
        {
            TransitionResource(commandList, backBuffer,
                D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

            FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

            ClearRTV(commandList, rtv, clearColor);
            ClearDepth(commandList, dsv);
        }

        commandList->SetPipelineState(m_PipelineState.Get());
        commandList->SetGraphicsRootSignature(m_RootSignature.Get());

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
        commandList->IASetIndexBuffer(&m_IndexBufferView);

        commandList->RSSetViewports(1, &m_Viewport);
        commandList->RSSetScissorRects(1, &m_ScissorRect);

        commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

        // Update the MVP matrix
        XMMATRIX mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_ViewMatrix);
        mvpMatrix = XMMatrixMultiply(mvpMatrix, m_ProjectionMatrix);
        commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

        commandList->DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);
*/
        batch->clear(clearColor, swapchain, currentIndex);

        batch->resourceBarrierTransition(
            poco::Batch::BarrierFlag::NONE,
            poco::Batch::ResourceState::RENDER_TARGET,
            poco::Batch::ResourceState::PRESENT,
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
