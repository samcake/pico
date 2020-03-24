// pico_six.cpp 
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
#include <pico/gpu/Descriptor.h>
#include <pico/gpu/Pipeline.h>
#include <pico/gpu/Batch.h>
#include <pico/gpu/Swapchain.h>

#include <pico/render/Renderer.h>
#include <pico/render/Camera.h>
#include <pico/render/Mesh.h>
#include <pico/render/Scene.h>
#include <pico/render/Viewport.h>

#include <pico/content/PointCloud.h>
#include <pico/drawables/PointcloudDrawable.h>

#include <vector>

//--------------------------------------------------------------------------------------

document::PointCloudPointer createPointCloud(const std::string& filepath) {
    return document::PointCloud::createFromPLY(filepath);
}

//--------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{

    std::string cloudPointFile("../asset/dragonStandRight_0.ply");
    if (argc > 1) {
        cloudPointFile = std::string(argv[argc - 1]);
    }

    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Create the pico api
    pico::ApiInit pico_init{ };
    auto result = pico::api::create(pico_init);

    if (!result) {
        std::clog << "Pico api failed to create ?" << std::endl;
        return 1;
    }

    // Some content, why not a pointcloud ?
    auto pointCloud = createPointCloud(cloudPointFile);

    // First a device, aka the gpu api used by pico
    pico::DeviceInit deviceInit {};
    auto gpuDevice = pico::Device::createDevice(deviceInit);

    // Second a Scene
    auto scene = std::make_shared<pico::Scene>();
  
    // A Camera to look at the scene
    auto camera = std::make_shared<pico::Camera>();
    camera->setViewport(1280.0f, 720.0f, true); // setting the viewport size, and yes adjust the aspect ratio
    camera->setOrientationFromRightUp({ 1.f, 0.f, 0.0f }, { 0.f, 1.f, 0.f });
    camera->setProjectionHeight(0.1f);
    camera->setFocal(0.1f);
    float cameraOrbitLength = 1.0f;

    // The viewport managing the rendering of the scene from the camera
    auto viewport = std::make_shared<pico::Viewport>(scene, camera, gpuDevice);

    // a drawable from the pointcloud
    auto pointCloudDrawable = std::make_shared<pico::PointCloudDrawable>();
  //  pointCloudDrawable->allocateDocumentDrawcallObject(gpuDevice, camera, pointCloud);
 //   auto item = scene->createItem(pointCloudDrawable);

    // Content creation
    float doAnimate = 1.0f;

    // Scene sphere:
    auto zoomToScene = [scene, camera, &cameraOrbitLength] ()
    {
        auto item = scene->getValidItemAt(1);
        if (!item.isValid() || scene->getItems().size() <= 1) {
            cameraOrbitLength = 10.0f;
            camera->setEye(/* from world origin*/ camera->getBack() * cameraOrbitLength);
            camera->setFar(10.0f * cameraOrbitLength);
        } else {
            // Configure the Camera to look at the scene
            auto bounds = item.getDrawable()->_bounds;
            auto cloudCenter = bounds.midPos();
            auto cloudHalfSize = (bounds.maxPos() - bounds.minPos());
            auto cloudHalfDiag = sqrt(core::dot(cloudHalfSize, cloudHalfSize));

            cameraOrbitLength = cloudHalfDiag / 2.0f; // zoom a bunch; the scanner always produces noise close to the camera that offsets the "real" boundaries
            camera->setEye(cloudCenter + camera->getBack() * cameraOrbitLength);
            camera->setFar(10.0f * cameraOrbitLength);
        }
    };
 
    // Presentation creation

    // We need a window where to present, let s use the pico::Window for convenience
    // This could be any window, we just need the os handle to create the swapchain next.
    auto windowHandler = new pico::WindowHandlerDelegate();
    pico::WindowInit windowInit { windowHandler };
    auto window = pico::Window::createWindow(windowInit);

    pico::SwapchainInit swapchainInit { (uint32_t) camera->getViewportWidth(), (uint32_t)camera->getViewportHeight(), (HWND) window->nativeWindow(), true };
    auto swapchain = gpuDevice->createSwapchain(swapchainInit);

    //Now that we have created all the elements, 
    // We configure the windowHandler onPaint delegate of the window to do real rendering!
    windowHandler->_onPaintDelegate = ([swapchain, viewport](const pico::PaintEvent& e) {
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
        viewport->present(swapchain);
    });

    // On resize deal with it
    windowHandler->_onResizeDelegate = [&](const pico::ResizeEvent& e) {
        // only resize the swapchain when we re done with the resize
        if (e.over) {
            gpuDevice->resizeSwapchain(swapchain, e.width, e.height);
        }

        // aspect ratio changes with a resize always
        // viewport resolution, only once the resize is over
        if (!e.over) {
            camera->setAspectRatio(e.width / (float)e.height);
        }
        else {
            camera->setViewport(e.width, e.height, true);
        }
    };

    windowHandler->_onKeyboardDelegate = [&](const pico::KeyboardEvent& e) {
        if (e.state && e.key == pico::KEY_SPACE) {
            doAnimate = (doAnimate == 0.f ? 1.0f : 0.0f);
        }
        if (e.state && e.key == pico::KEY_1) {
            // look side
            camera->setOrientationFromRightUp({ 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.0f });
            zoomToScene();//camera->zoomTo(sceneSphere);
        }

        if (e.state && e.key == pico::KEY_2) {
            // look lateral
            camera->setOrientationFromRightUp({ 0.f, 0.f, -1.f }, { 0.f, 1.f, 0.0f });
            zoomToScene();//camera->zoomTo(sceneSphere);
        }

        if (e.state && e.key == pico::KEY_3) {
            // look down
            camera->setOrientationFromRightUp({ 1.f, 0.f, 0.f }, { 0.f, 0.f, -1.f });
            zoomToScene();//camera->zoomTo(sceneSphere);
        }

        if (e.state && e.key == pico::KEY_4) {
            // look 3/4 down
            camera->setOrientationFromRightUp({ 1.f, 0.f, -1.f }, { 0.f, 1.f, -1.0f });
            zoomToScene();//camera->zoomTo(sceneSphere);
        }
    };

    windowHandler->_onMouseDelegate = [&](const pico::MouseEvent& e) {
        if (e.state & pico::MOUSE_MOVE) {
            if (e.state & pico::MOUSE_RBUTTON) {
                float orbitScale = 0.01f;
                camera->orbit(cameraOrbitLength, e.delta.x * orbitScale, -e.delta.y * orbitScale);
            }
            if (e.state & pico::MOUSE_MBUTTON) {
                float panScale = cameraOrbitLength * 0.001f;
                camera->pan(-e.delta.x* panScale, e.delta.y * panScale);
            }
        } else if (e.state & pico::MOUSE_WHEEL) {
            if (e.state & pico::MOUSE_CONTROL) {
                float zoomScale = 0.1f;
                camera->setFocal( camera->getFocal() * (1.0f +  e.wheel * zoomScale));
            } else {
                float dollyScale = 0.05f;
                cameraOrbitLength *= (1.0f + e.wheel * dollyScale);
                zoomToScene();//camera->zoomTo(sceneSphere);
  //              camera->zoomTo(sceneSphere);
            }
        }
    };

    // Render Loop 
    bool keepOnGoing = true;
    while (keepOnGoing) {
        keepOnGoing = window->messagePump();
    }

    pico::api::destroy();
    std::clog << "Pico api destroyed" << std::endl;

     return 0;
}
