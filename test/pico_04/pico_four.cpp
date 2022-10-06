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

#include <graphics/drawables/GizmoDrawable.h>

#include <graphics/drawables/PrimitiveDrawable.h>

#include <graphics/drawables/DashboardDrawable.h>


#include <uix/Window.h>
#include <uix/CameraController.h>

#include <vector>

//--------------------------------------------------------------------------------------
// pico 4: Scene, Viewport and Camera to render a simple 3d scene
// introducing:
// render::Scene
// render::Camera
// render::Viewport
// drawable::PointcloudDrawable
// uix::CameraController
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
    auto pointCloud = document::PointCloud::createFromJSON(cloudPointFile);
    auto triangleSoup = document::TriangleSoup::createFromPLY(triangleSoupFile);

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


    // A point cloud drawable factory
    auto pointCloudDrawableFactory = std::make_shared<graphics::PointCloudDrawableFactory>();
    pointCloudDrawableFactory->allocateGPUShared(gpuDevice);

    // a drawable from the pointcloud
    graphics::PointCloudDrawable* pointCloudDrawable(pointCloudDrawableFactory->createPointCloudDrawable(gpuDevice, pointCloud));
    pointCloudDrawableFactory->allocateDrawcallObject(gpuDevice, scene, *pointCloudDrawable);
    auto pcdrawable = scene->createDrawable(*pointCloudDrawable);

    // A triangel soup drawable factory
    auto triangleSoupDrawableFactory = std::make_shared<graphics::TriangleSoupDrawableFactory>();
    triangleSoupDrawableFactory->allocateGPUShared(gpuDevice);

    // a drawable from the trianglesoup
    graphics::TriangleSoupDrawable* triangleSoupDrawable(triangleSoupDrawableFactory->createTriangleSoupDrawable(gpuDevice, triangleSoup));
    triangleSoupDrawableFactory->allocateDrawcallObject(gpuDevice, scene, *triangleSoupDrawable);
    auto tsdrawable = scene->createDrawable(*triangleSoupDrawable);

    // A gizmo drawable factory
    auto gizmoDrawableFactory = std::make_shared<graphics::GizmoDrawableFactory>();
    gizmoDrawableFactory->allocateGPUShared(gpuDevice);

    // Some nodes to layout the scene and animate objects
    auto node0 = scene->createNode(core::mat4x3(), -1);
 
    auto rnode = scene->createNode(core::translation(core::vec3(4.0f, 0.0f, 0.0f)), node0.id());

    auto bnode = scene->createNode(core::translation(core::vec3(8.0f, 0.0f, 0.0f)), rnode.id());

    auto cnode = scene->createNode(core::translation(core::vec3(0.0f, 5.0f, 0.0f)), bnode.id());

    auto dnode = scene->createNode(core::translation(core::vec3(0.0f, 0.0f, 3.0f)), cnode.id());

    auto enode = scene->createNode(core::translation(core::vec3(0.0f, 1.0f, 4.0f)), rnode.id());

    //  node0
    //    +---- rnode
    //            +---- bnode
    //            |       +---- cnode
    //            |               +---- dnode
    //            +---- enode            
    //

    // Some items unique instaces of the drawable and the specified nodes
    auto pcitem = scene->createItem(node0, pcdrawable);

    auto tsitem = scene->createItem(enode, tsdrawable);

    auto pcitem2 = scene->createItem(cnode, pcdrawable);

    auto tsitem2 = scene->createItem(dnode, tsdrawable);




    
    // A Primitive drawable factory
    auto primitiveDrawableFactory = std::make_shared<graphics::PrimitiveDrawableFactory>();
    primitiveDrawableFactory->allocateGPUShared(gpuDevice);

    // a Primitive
    auto p_drawable = scene->createDrawable(*primitiveDrawableFactory->createPrimitive(gpuDevice));
    primitiveDrawableFactory->allocateDrawcallObject(gpuDevice, scene, p_drawable.as<graphics::PrimitiveDrawable>());
    p_drawable.as<graphics::PrimitiveDrawable>()._size = {1.0, 2.0, 0.7 };

    std::vector<graphics::NodeID> prim_nodes;
    {
        int width = 100;
        float space = 4.0f;
        float pos_offset = width / 2 * space;
        for (int i = 0; i < width * width; ++i) {
            float t = acos(-1.0f) * i / float(width * width);
            auto p_node = scene->createNode(
                core::translation_rotation(
                    core::vec3(-space * (i % width) + pos_offset, -1.0f, space * (i / width) - pos_offset),
                    core::rotor3(core::vec3::X, core::vec3(cos(t), 0, sin(t)))
                ),
                rnode.id());
            // node0.id());
            auto p_item = scene->createItem(p_node, p_drawable);
            prim_nodes.push_back(p_node.id());
        }
    }

    //  node0
    //    +---- rnode
    //            +---- bnode
    //            |       +---- cnode
    //            |               +---- dnode
    //            +---- enode            
    //            |
    //            +---- pnode 0 
    //            +---- pnode 0  
    //            +---- pnode ...  
    //            +---- pnode (width * width - 1)  


    // a gizmo drawable to draw the transforms
    auto gzdrawable_node = scene->createDrawable(*gizmoDrawableFactory->createNodeGizmo(gpuDevice));
    gizmoDrawableFactory->allocateDrawcallObject(gpuDevice, scene, gzdrawable_node.as<graphics::NodeGizmo>());
    gzdrawable_node.as<graphics::NodeGizmo>().nodes.resize(6);

    auto gzdrawable_item = scene->createDrawable(*gizmoDrawableFactory->createItemGizmo(gpuDevice));
    gizmoDrawableFactory->allocateDrawcallObject(gpuDevice, scene, gzdrawable_item.as<graphics::ItemGizmo>());
    gzdrawable_item.as<graphics::ItemGizmo>().items.resize(100);


    auto gzitem_item = scene->createItem(graphics::Node::null, gzdrawable_item);
    gzitem_item.setVisible(false);
    auto gzitem_node = scene->createItem(graphics::Node::null, gzdrawable_node);
    gzitem_node.setVisible(false);

    // A dashboard factory and drawable to represent some debug data
    auto dashboardDrawableFactory = std::make_shared<graphics::DashboardDrawableFactory>();
    dashboardDrawableFactory->allocateGPUShared(gpuDevice);

    // a dashboard
    auto dashboard_drawable = scene->createDrawable(*dashboardDrawableFactory->createDrawable(gpuDevice));
    dashboardDrawableFactory->allocateDrawcallObject(gpuDevice, scene, dashboard_drawable.as<graphics::DashboardDrawable>());

    auto dashboard_item = scene->createItem(graphics::Node::null, dashboard_drawable);
    dashboard_item.setVisible(false);

    scene->_nodes.updateTransforms();


    scene->updateBounds();
    core::vec4 sceneSphere = scene->getBounds().toSphere();
    
    // Content creation
    float doAnimate = 0.0f;


    // For user input, we can use a CameraController which will provide a standard manipulation of the camera from keyboard and mouse
    auto camControl = std::make_shared< uix::CameraController >(camera);
    float cameraOrbitLength = camControl->zoomTo(sceneSphere);
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

    graphics::SwapchainInit swapchainInit{ (HWND)window->nativeWindow(), window->width(), window->height(), true };
    auto swapchain = gpuDevice->createSwapchain(swapchainInit);

    // On resize deal with it
    windowHandler->_onResizeDelegate = [&](const uix::ResizeEvent& e) {
        // only resize the swapchain when we re done with the resize
        if (e.done) {
            gpuDevice->resizeSwapchain(swapchain, e.width, e.height);
        }

        camControl->onResize(e);
    };

    //Now that we have created all the elements, 
    // We configure the windowHandler onPaint delegate of the window to do real rendering!
    windowHandler->_onPaintDelegate = ([&](const uix::PaintEvent& e) {
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
        auto t = acos(-1.0f) * (currentSample._frameNum / 300.0f);

        if (doAnimate) {
            // Move something
            scene->_nodes.editTransform(rnode.id(), [t] (core::mat4x3& rts) -> bool {
                core::rotor3 rotor(core::vec3(1.0f, 0.0f, 0.0f), core::vec3(cos(-t), 0.0f, sin(-t)));
                core::rotation(rts, rotor);
                return true;
            });
            scene->_nodes.editTransform(bnode.id(), [t](core::mat4x3& rts) -> bool {
                return true;
            });
            scene->_nodes.editTransform(cnode.id(), [t](core::mat4x3& rts) -> bool {
                core::rotor3 rotor(core::vec3(1.0f, 0.0f, 0.0f), core::vec3(cos(0.2 * t), 0.0f, sin(0.2 * t)));
                core::rotation(rts, rotor);
                return true;
            });
            scene->_nodes.editTransform(dnode.id(), [t](core::mat4x3& rts) -> bool {
                core::rotor3 rotor(core::vec3(1.0f, 0.0f, 0.0f), core::vec3(cos(0.5 * t), sin(0.5 * t), 0.0f));
                core::rotation(rts, rotor);
                return true;
            });

            for (auto prim_node : prim_nodes) {
                scene->_nodes.editTransform(prim_node, [t](core::mat4x3& rts) -> bool {
                    core::rotor3 rotor(core::vec3(1.0f, 0.0f, 0.0f), core::vec3(cos(0.005 * t), 0.0f, sin(0.005 * t)));
                    core::rotate(rts, rotor);
                    return true;
                });
            }
        }

        camControl->update(std::chrono::duration_cast<std::chrono::microseconds>(frameSample._frameDuration));

        // Render!
        viewport->present(swapchain);
    });

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
