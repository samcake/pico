// pico_one.cpp 
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

#include <chrono>
#include <iostream>

#include <core/api.h>

#include <graphics/gpu/Device.h>
#include <graphics/gpu/Swapchain.h>
#include <graphics/render/Renderer.h>

#include <uix/Window.h>


//--------------------------------------------------------------------------------------
// pico 1: Clear a swapchain
// introducing:
// gpu::Device
// gpu::Swapchain
// render::Renderer
// uix::Window


//--------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Create the pico api
    core::ApiInit pico_init{ };
    auto result = core::api::create(pico_init);

    if (!result) {
        std::clog << "Pico api failed to create ?" << std::endl;
        return 1;
    }

  /*  result = pico::api::create(pico_init);
    if (!result) {
        std::clog << "Pico api failed to create on second attempt, yes?" << std::endl;
    }*/

    // Content creation

    // Renderer creation

    // First a device, aka the gpu api used by pico
    graphics::DeviceInit deviceInit {};
    auto gpuDevice = graphics::Device::createDevice(deviceInit);

    // Next, a renderer built on this device
    // the default renderer without any further configuration will do
    //  a clear color of the swapchain passed in the render call
    auto renderer = std::make_shared<graphics::Renderer>(gpuDevice, nullptr);

    // Presentation creation

    // We need a window where to present, let s use the pico::Window for convenience
    // This could be any window, we just need the os handle to create the swapchain next.
    auto windowHandler = new uix::WindowHandlerDelegate();
    uix::WindowInit windowInit { windowHandler };
    auto window = uix::Window::createWindow(windowInit);

    graphics::SwapchainInit swapchainInit { 640, 480, (HWND) window->nativeWindow() };
    auto swapchain = gpuDevice->createSwapchain(swapchainInit);


    //Now that we have created all the elements, 
    // We configure the windowHandler onPaint delegate of the window to do real rendering!
    windowHandler->_onPaintDelegate = ([swapchain, renderer](const uix::PaintEvent& e) {
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
        // By default just clear the swapchain
        renderer->render(nullptr, swapchain);
    });

    // Render Loop 
    bool keepOnGoing = true;
    while (keepOnGoing) {
        keepOnGoing = window->messagePump();

    }

    core::api::destroy();
    std::clog << "Pico api destroyed" << std::endl;

     return 0;
}
