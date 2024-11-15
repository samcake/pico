// pico_four.cpp 
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

#include <core/api.h>

#include <graphics/gpu/Device.h>
#include <graphics/gpu/Resource.h>
#include <graphics/gpu/Shader.h>
#include <graphics/gpu/Descriptor.h>
#include <graphics/gpu/Pipeline.h>
#include <graphics/gpu/Batch.h>
#include <graphics/gpu/Swapchain.h>

#include <graphics/render/Renderer.h>
#include <graphics/render/Camera.h>
#include <graphics/render/Mesh.h>
#include <graphics/render/Draw.h>
#include <graphics/render/Scene.h>
#include <graphics/render/Viewport.h>


#include <graphics/drawables/GizmoDraw.h>

#include <graphics/drawables/PrimitiveDraw.h>

#include "ocean.h"

#include <uix/Window.h>
#include <uix/CameraController.h>

#include <vector>

//--------------------------------------------------------------------------------------
// pico ocean:
// Inspired by... Massinissa Mokhtari
// https://github.com/massile/ocean
// A reinplementation of his wonderful sample
// 
//--------------------------------------------------------------------------------------


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

    // First a device, aka the gpu api used by pico
    graphics::DeviceInit deviceInit {};
    auto gpuDevice = graphics::Device::createDevice(deviceInit);

    // Second a Scene
    auto scene = std::make_shared<graphics::Scene>(graphics::SceneInit{gpuDevice});
  
    // A Camera to look at the scene
    auto camera = scene->createCamera();
    camera->setViewport(1280.0f, 720.0f, true); // setting the viewport size, and yes adjust the aspect ratio
    camera->setOrientationFromRightUp({ 1.f, 0.f, 0.0f }, { 0.f, 1.f, 0.f });
    camera->setProjectionHeight(0.1f);
    camera->setFocal(0.1f);

    // The viewport managing the rendering of the scene from the camera
    auto viewport = std::make_shared<graphics::Viewport>(graphics::ViewportInit{ scene, gpuDevice, nullptr, camera->id() });

    // Some nodes to layout the scene and animate objects
    auto node0 = scene->createNode({});

    auto ocean_resolution = 512;
    generateSpectra(ocean_resolution, 1.0f, 1024, 1.0f, gpuDevice, scene, camera, node0);

    // Create gizmos to draw the node transform and item tree
    auto [gznode_tree, gzitem_tree] = graphics::GizmoDraw_createSceneGizmos(scene, gpuDevice);

    // scene Bounds
    scene->updateBounds();
    core::vec4 sceneSphere = scene->getBounds().toSphere();
    
    // Content creation
    float doAnimate = 0.0f;


    // For user input, we can use a CameraController which will provide a standard manipulation of the camera from keyboard and mouse
    auto camControl = std::make_shared< uix::CameraController >(camera);
    float cameraOrbitLength = camControl->zoomTo(sceneSphere);
    camera->setFar(cameraOrbitLength * 100.0f);

    // Presentation creation

    // We need a window where to present, let s use the graphics::Window for convenience
    // This could be any window, we just need the os handle to create the swapchain next.
    auto windowHandler = new uix::WindowHandlerDelegate();
    uix::WindowInit windowInit { windowHandler, "Pico Ocean" };
    auto window = uix::Window::createWindow(windowInit);
    camera->setViewport(window->width(), window->height(), true); // setting the viewport size, and yes adjust the aspect ratio

    graphics::SwapchainInit swapchainInit { (HWND)window->nativeWindow(), window->width(), window->height(), true };
    auto swapchain = gpuDevice->createSwapchain(swapchainInit);


    //Now that we have created all the elements, 
    // We configure the windowHandler onPaint delegate of the window to do real rendering!
    windowHandler->_onPaintDelegate = ([&](const uix::PaintEvent& e) {
        // Measuring framerate
        static core::FrameTimer::Sample frameSample;
        auto currentSample = viewport->lastFrameTimerSample();
        if ((currentSample._frameNum - frameSample._frameNum) > 60) {
            frameSample = currentSample;
            std::string title = std::string("Pico Ocean: ") + std::to_string((uint32_t) frameSample.beginRate())
                              + std::string("Hz ") + std::to_string(0.001f * frameSample._frameDuration.count()) + std::string("ms")
                              + (camera->isOrtho()
                                    ? (std::string(" ortho:") + std::to_string((int)(1000.0f * camera->getOrthoHeight())) + std::string("mm"))
                                    : (std::string(" fov:") + std::to_string((int)(camera->getFovDeg())) + std::string("deg")) );
            window->setTitle(title);
        }
        auto t = (currentSample._frameNum / 90.0f);
        auto ts = acos(-1.0f) * (currentSample._frameNum / 300.0f);

        if (doAnimate) {
            updateHeights(t);
        }

        camControl->update(std::chrono::duration_cast<std::chrono::microseconds>(frameSample._frameDuration));

        // Render!
        viewport->present(swapchain);
    });

    // On resize deal with it
    windowHandler->_onResizeDelegate = [&](const uix::ResizeEvent& e) {
        // only resize the swapchain when we re done with the resize
        if (e.done) {
            gpuDevice->resizeSwapchain(swapchain, e.width, e.height);
        }

        camControl->onResize(e);
    };

    windowHandler->_onKeyboardDelegate = [&](const uix::KeyboardEvent& e) {
        if (e.state && e.key == uix::KEY_SPACE) {
            doAnimate = (doAnimate == 0.f ? 1.0f : 0.0f);
        }

        if (e.state && e.key == uix::KEY_N) {
            gznode_tree.setVisible(!gznode_tree.isVisible());
        }
        if (e.state && e.key == uix::KEY_B) {
            gzitem_tree.setVisible(!gzitem_tree.isVisible());
        }

        bool zoomToScene = false;
        if (e.state && e.key == uix::KEY_1) {
            // look side
            camera->setOrientationFromRightUp({ 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.0f });
            zoomToScene = true;
        }

        if (zoomToScene) {
            camControl->zoomTo(sceneSphere);
        }
        if (camControl->onKeyboard(e)) {
            return;
        }
    };

    windowHandler->_onMouseDelegate = [&](const uix::MouseEvent& e) {

        if (camControl->onMouse(e)) {
            return;
        }

        if (e.state & uix::MOUSE_MOVE) {
            if (e.state & uix::MOUSE_LBUTTON) {
            }
            if (e.state & uix::MOUSE_MBUTTON) {
            }
        }
    };
 
    // Render Loop 
    bool keepOnGoing = true;
    while (keepOnGoing) {
        keepOnGoing = window->messagePump();
    }

    core::api::destroy();

    return 0;
}
