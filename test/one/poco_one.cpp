// poco_one.cpp 
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

  /*  result = poco::api::create(poco_init);
    if (!result) {
        std::clog << "Poco api failed to create on second attempt, yes?" << std::endl;
    }*/

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

    // Next, a renderer built on this device
    auto renderer = std::make_shared<poco::Renderer>(gpuDevice);


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

/*
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{ 
    return 0;
}*/