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
#include <graphics/render/Drawable.h>
#include <graphics/render/Scene.h>
#include <graphics/render/Viewport.h>

#include <document/PointCloud.h>
#include <graphics/drawables/PointcloudDrawable.h>


#include <document/TriangleSoup.h>
#include <graphics/drawables/TriangleSoupDrawable.h>

#include <uix/Window.h>

#include <vector>

//--------------------------------------------------------------------------------------
// pico 4: Scene, Viewport and Camera to render a simple 3d scene
// introducing:
// render::Scene
// render::Camera
// render::Viewport
// drawable::PointcloudDrawable
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{

 //   std::string cloudPointFile("../asset/pointcloud/AsianElephantPointcloud.ply");
    std::string cloudPointFile("../asset/pointcloud/mammoth.json");
    if (argc > 1) {
        cloudPointFile = std::string(argv[argc - 1]);
    }

   // std::string triangleSoupFile("../asset/trianglesoup/meshlab_skull.ply");
    std::string triangleSoupFile("../asset/trianglesoup/cow.ply");
  //  std::string triangleSoupFile("../asset/trianglesoup/cow.binary.ply");
    if (argc > 1) {
        triangleSoupFile = std::string(argv[argc - 1]);
    }

    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Create the pico api
    core::ApiInit pico_init{ };
    auto result = core::api::create(pico_init);

    if (!result) {
        std::clog << "Pico api failed to create ?" << std::endl;
        return 1;
    }

    // Some content, why not a pointcloud ?
  //  auto pointCloud = document::PointCloud::createFromPLY(cloudPointFile);
    auto pointCloud = document::PointCloud::createFromJSON(cloudPointFile);
    auto triangleSoup = document::TriangleSoup::createFromPLY(triangleSoupFile);

    // First a device, aka the gpu api used by pico
    graphics::DeviceInit deviceInit {};
    auto gpuDevice = graphics::Device::createDevice(deviceInit);

    // Second a Scene
    auto scene = std::make_shared<graphics::Scene>();
  
    // A Camera to look at the scene
    auto camera = std::make_shared<graphics::Camera>();
    camera->setViewport(1280.0f, 720.0f, true); // setting the viewport size, and yes adjust the aspect ratio
    camera->setOrientationFromRightUp({ 1.f, 0.f, 0.0f }, { 0.f, 1.f, 0.f });
    camera->setProjectionHeight(0.1f);
    camera->setFocal(0.1f);

    // The viewport managing the rendering of the scene from the camera
    auto viewport = std::make_shared<graphics::Viewport>(scene, camera, gpuDevice);

    // A point cloud drawable factory
    auto pointCloudDrawableFactory = std::make_shared<graphics::PointCloudDrawableFactory>();
    pointCloudDrawableFactory->allocateGPUShared(gpuDevice);

    // a drawable from the pointcloud
    graphics::PointCloudDrawablePointer pointCloudDrawable(pointCloudDrawableFactory->createPointCloudDrawable(gpuDevice, pointCloud));
    pointCloudDrawableFactory->allocateDrawcallObject(gpuDevice, camera, pointCloudDrawable);
    auto pcitem = scene->createItem(pointCloudDrawable);

    // a drawable from the trianglesoup
    auto triangleSoupDrawable = std::make_shared<graphics::TriangleSoupDrawable>();
    triangleSoupDrawable->allocateDocumentDrawcallObject(gpuDevice, camera, triangleSoup);
 //   auto tsitem = scene->createItem(triangleSoupDrawable);

    // Content creation
    float doAnimate = 1.0f;

    core::vec4 sceneSphere(0.0f, 0.0f, 0.0f, 10.0f);
    {
        auto item = scene->getValidItemAt(0);
        if (item.isValid()) {
            // Configure the Camera to look at the scene
            auto bounds = item.getDrawable()->_bounds;
            auto cloudCenter = bounds.midPos();
            auto cloudHalfSize = (bounds.maxPos() - bounds.minPos());
            auto cloudHalfDiag = sqrt(core::dot(cloudHalfSize, cloudHalfSize));
            sceneSphere = core::vec4(cloudCenter, cloudHalfDiag);
        }
    }
    float cameraOrbitLength = camera->zoomTo(sceneSphere);
    camera->setFar(cameraOrbitLength * 100.0f);
    
 
    bool editPointCloudSize = false;
    bool editPointCloudPerspectiveSpriteX = false;
    bool editPointCloudPerspectiveDepth = false;
    bool editPointCloudShowPerspectiveDepth = false;

    // Presentation creation

    // We need a window where to present, let s use the graphics::Window for convenience
    // This could be any window, we just need the os handle to create the swapchain next.
    auto windowHandler = new uix::WindowHandlerDelegate();
    uix::WindowInit windowInit { windowHandler, "Pico 4" };
    auto window = uix::Window::createWindow(windowInit);
    camera->setViewport(window->width(), window->height(), true); // setting the viewport size, and yes adjust the aspect ratio

    graphics::SwapchainInit swapchainInit { window->width(), window->height(), (HWND) window->nativeWindow(), true };
    auto swapchain = gpuDevice->createSwapchain(swapchainInit);

    //Now that we have created all the elements, 
    // We configure the windowHandler onPaint delegate of the window to do real rendering!
    windowHandler->_onPaintDelegate = ([swapchain, viewport, window, camera](const uix::PaintEvent& e) {
        // Measuring framerate
        static core::FrameTimer::Sample frameSample;
        auto currentSample = viewport->lastFrameTimerSample();
        if ((currentSample._frameNum - frameSample._frameNum) > 60) {
            frameSample = currentSample;
            std::string title = std::string("Pico 4: ") + std::to_string((uint32_t) frameSample.beginRate())
                              + std::string("Hz ") + std::to_string(0.001f * frameSample._frameDuration.count()) + std::string("ms")
                              + (camera->isOrtho()
                                    ? (std::string(" ortho:") + std::to_string((int)(1000.0f * camera->getOrthoHeight())) + std::string("mm"))
                                    : (std::string(" fov:") + std::to_string((int)(camera->getFovDeg())) + std::string("deg")) );
            window->setTitle(title);
        }
 
        // Render!
        viewport->present(swapchain);
    });

    // On resize deal with it
    windowHandler->_onResizeDelegate = [&](const uix::ResizeEvent& e) {
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

    windowHandler->_onKeyboardDelegate = [&](const uix::KeyboardEvent& e) {
        if (e.state && e.key == uix::KEY_SPACE) {
            doAnimate = (doAnimate == 0.f ? 1.0f : 0.0f);
        }

        if (e.key == uix::KEY_O) {
            editPointCloudSize = e.state;
        }
        if (e.key == uix::KEY_I) {
            editPointCloudPerspectiveSpriteX = e.state;
        }
        if (e.key == uix::KEY_U) {
            editPointCloudPerspectiveDepth = e.state;
        }
        if (e.key == uix::KEY_J) {
            editPointCloudShowPerspectiveDepth = e.state;
        }

        if (e.state && e.key == uix::KEY_P) {
            // look side
            camera->setOrtho(!camera->isOrtho());
        }

        bool zoomToScene = false;
        if (e.state && e.key == uix::KEY_1) {
            // look side
            camera->setOrientationFromRightUp({ 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.0f });
            zoomToScene = true;
        }

        if (e.state && e.key == uix::KEY_2) {
            // look lateral
            camera->setOrientationFromRightUp({ 0.f, 0.f, -1.f }, { 0.f, 1.f, 0.0f });
            zoomToScene = true;
        }

        if (e.state && e.key == uix::KEY_3) {
            // look down
            camera->setOrientationFromRightUp({ 1.f, 0.f, 0.f }, { 0.f, 0.f, -1.f });
            zoomToScene = true;
        }

        if (e.state && e.key == uix::KEY_4) {
            // look 3/4 down
            camera->setOrientationFromRightUp({ 1.f, 0.f, -1.f }, { 0.f, 1.f, -1.0f });
            zoomToScene = true;
        }

        if (zoomToScene) {
            cameraOrbitLength = camera->zoomTo(sceneSphere);
        }
    };

    windowHandler->_onMouseDelegate = [&](const uix::MouseEvent& e) {
        if (e.state & uix::MOUSE_MOVE) {
            if (e.state & uix::MOUSE_LBUTTON) {
                if (pointCloudDrawableFactory) {
                    if (editPointCloudSize) {
                        auto v = pointCloudDrawableFactory->getUniforms().getSpriteSize();
                        v -= e.delta.y * 0.01f;
                        pointCloudDrawableFactory->editUniforms().setSpriteSize(v);
                    }
                    if (editPointCloudPerspectiveSpriteX) {
                        auto v = pointCloudDrawableFactory->getUniforms().getPerspectiveSprite();
                        v = e.pos.x / ((float)window->width());
                        pointCloudDrawableFactory->editUniforms().setPerspectiveSprite(v);
                    }
                    if (editPointCloudPerspectiveDepth) {
                        auto v = pointCloudDrawableFactory->getUniforms().getPerspectiveDepth();
                        v -= e.delta.y * 0.01f;
                        pointCloudDrawableFactory->editUniforms().setPerspectiveDepth(v);
                    }
                    if (editPointCloudShowPerspectiveDepth) {
                        auto v = pointCloudDrawableFactory->getUniforms().getShowPerspectiveDepthPlane();
                        v = e.pos.x / ((float)window->width());
                        pointCloudDrawableFactory->editUniforms().setShowPerspectiveDepthPlane(v);
                    }
                }
            }
            if (e.state & uix::MOUSE_MBUTTON) {
                // Center of the screen is Focal = SensorHeight
                float ny = (0.5f + e.pos.y / (float)window->height());
                if (camera->isOrtho()) {
                } else {
                    camera->setFocal(camera->getProjectionHeight()* (ny* ny* ny));
                }
            }
            if (e.state & uix::MOUSE_RBUTTON) {
                if (e.state & uix::MOUSE_CONTROL) {
                    float panScale = cameraOrbitLength * 0.001f;
                    if (camera->isOrtho()) {
                        panScale = camera->getOrthoHeight() / camera->getViewportHeight();
                    } else {
                    }    
                    camera->pan(-e.delta.x * panScale, e.delta.y * panScale);
                } else {
                    float orbitScale = 0.01f;
                    camera->orbit(cameraOrbitLength, orbitScale * (float)e.delta.x, orbitScale * (float)-e.delta.y);
                }
            }
        } else if (e.state & uix::MOUSE_WHEEL) {
            if (e.state & uix::MOUSE_CONTROL) {
                if (camera->isOrtho()) {
                    float dollyScale = 0.08f;
                    float orbitLengthDelta = (e.wheel * dollyScale) * cameraOrbitLength;
                    cameraOrbitLength = camera->boom(cameraOrbitLength, orbitLengthDelta);
                } else {
                    float zoomScale = 0.1f;
                    camera->setFocal(camera->getFocal() * (1.0f +  e.wheel * zoomScale));
                }
            } else {
                if (camera->isOrtho()) {
                    float zoomScale = 0.1f;
                    camera->setOrthoHeight(camera->getOrthoHeight() * (1.0f + e.wheel * zoomScale));
                } else {
                    float dollyScale = 0.08f;
                    float orbitLengthDelta = (e.wheel * dollyScale) * cameraOrbitLength;
                    cameraOrbitLength = camera->boom(cameraOrbitLength, orbitLengthDelta);
                }
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
