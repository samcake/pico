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


#include <document/PointCloud.h>
#include <graphics/drawables/PointcloudDraw.h>


#include <document/TriangleSoup.h>
#include <graphics/drawables/TriangleSoupDraw.h>

#include <graphics/drawables/GizmoDraw.h>

#include <graphics/drawables/PrimitiveDraw.h>

#include <graphics/drawables/DashboardDraw.h>

#include <graphics/drawables/ModelDraw.h>

#include <uix/Window.h>
#include <uix/CameraController.h>

#include <vector>

//--------------------------------------------------------------------------------------
// pico 4: Scene, Viewport and Camera to render a simple 3d scene
// introducing:
// render::Scene
// render::Camera
// render::Viewport
// draw::PointcloudDraw
// uix::CameraController
//--------------------------------------------------------------------------------------


struct AppState {

    graphics::ModelDrawFactoryPointer _modelDrawFactory;
    graphics::ModelDrawUniformsPointer _modelDrawParams;


    graphics::ScenePointer scene;
    struct {
        graphics::NodeID rootNodeID;
        graphics::ItemID modelItemID;
    } models;

    struct {
        graphics::CameraID _current = graphics::INVALID_CAMERA_ID;
    } cams;


    core::vec3 _modelInsertOffset;

    struct {
        graphics::Item tree_node;
        graphics::Item tree_item;
        graphics::Item dashboard;

        bool sky_ui{ true };
    } tools;

};

AppState state;


graphics::NodeIDs generateModel(document::ModelPointer lmodel, graphics::DevicePointer& gpuDevice, graphics::ScenePointer& scene, graphics::NodeID nodeRoot) {

    if (!state._modelDrawFactory) {
        state._modelDrawFactory = std::make_shared<graphics::ModelDrawFactory>(gpuDevice);
        state._modelDrawParams = state._modelDrawFactory->getUniformsPtr();
    }

    graphics::ItemIDs modelItemIDs;
    if (lmodel) {
        auto modelDrawPtr = state._modelDrawFactory->createModel(gpuDevice, lmodel);
        state._modelDrawFactory->allocateDrawcallObject(gpuDevice, scene, *modelDrawPtr);


        modelItemIDs = state._modelDrawFactory->createModelParts(nodeRoot, scene, *modelDrawPtr);

        // let's offset the root to not overlap on previous model
        if (modelItemIDs.size()) {
            auto modelRootNodeId = scene->getItem(modelItemIDs[0]).nodeID();

            auto modelBound = modelDrawPtr->getBound();
            auto minCorner = modelBound.minPos();
            auto maxCorner = modelBound.maxPos();
            auto boundCenter = modelBound.center;

            auto modelOffset = modelBound.half_size * core::vec3(1.0, 0.0, 1.0) * (1.0 + 0.1);
            auto modelPos = state._modelInsertOffset + modelOffset;
            modelPos.y += modelBound.half_size.y;


            modelPos = modelPos - boundCenter;

            scene->_nodes.editNodeTransform(modelRootNodeId, [modelPos](core::mat4x3& rts) -> bool {
                core::translation(rts, modelPos);
                return true;
                });

            state._modelInsertOffset = state._modelInsertOffset + modelOffset * 2.0;
        }
    }
    else {
        picoLog("Model document is empty");
    }

    return modelItemIDs;
}


document::ModelPointer loadModel() {
    document::ModelPointer lmodel;
    //  std::string modelFile("../asset/gltf/toycar/toycar.gltf");
    //   std::string modelFile("../asset/gltf/AntiqueCamera.gltf");
   //    std::string modelFile("../asset/gltf/Sponza.gltf");
   // std::string modelFile("../asset/gltf/WaterBottle/WaterBottle.gltf");
    // std::string modelFile("../asset/gltf/Lantern/lantern.gltf");
      std::string modelFile("../asset/gltf/buggy.gltf");
    //   std::string modelFile("../asset/gltf/VC.gltf");
    //  std::string modelFile("../asset/gltf/Duck/duck.gltf");
   //  std::string modelFile("../asset/gltf/OrientationTest.gltf");
   // std::string modelFile("../asset/gltf/Boombox/BoomBoxWithAxes.gltf");
   // std::string modelFile("../asset/gltf/BoxVertexColors.gltf");
 //   std::string modelFile("../asset/gltf/Box.gltf");
  //  std::string modelFile("../asset/gltf/DamagedHelmet/DamagedHelmet.gltf");
  //  std::string modelFile("../asset/gltf/DamagedHelmet/DamagedHelmet - Copy.gltf");
    // std::string modelFile("../asset/gltf/DamagedHelmet/DamagedHelmet-embedded.gltf");

    //  std::string modelFile("../asset/gltf/Rusted Metal Barrel_teufceuda_Metal/Rusted Metal Barrel_LOD2__teufceuda.gltf");
    //  std::string modelFile("../asset/gltf/Castle Parapet Wall_sbxuw_3D Asset/Castle Parapet Wall_LOD0__sbxuw.gltf");
    // std::string modelFile("../asset/gltf/Half Avocado_ujcxeblva_3D Asset/Half Avocado_LOD0__ujcxeblva.gltf");

   // std::string modelFile("C:\\Megascans/Pico/Banana_vfendgyiw/Banana_LOD0__vfendgyiw.gltf");
   // std::string modelFile("C:\\Megascans/Pico/Nordic Beach Rock_uknoehp/Nordic Beach Rock_LOD0__uknoehp.gltf");
  //  std::string modelFile("C:\\Megascans/Pico/Sponza-intel/Main/NewSponza_Main_Blender_glTF.gltf");

    // std::string modelFile("C:\\Megascans/Pico/Test-fbx_f48bbc8f-9166-9bc4-fbfb-688a71b1baa7/Test-fbx_LOD0__f48bbc8f-9166-9bc4-fbfb-688a71b1baa7.gltf");
    // std::string modelFile("C:\\Megascans/Pico/Wooden Chair_uknjbb2bw/Wooden Chair_LOD0__uknjbb2bw.gltf");
    //  std::string modelFile("C:\\Megascans/Pico/Japanese Statue_ve1haetqx/Japanese Statue_LOD0__ve1haetqx.gltf");
    // std::string modelFile("C:\\Megascans/Pico/Fire Hydrant_uiohdaofa/Fire Hydrant_LOD0__uiohdaofa.gltf");
    // std::string modelFile("C:\\Megascans/Pico/Fire Hydrant_uh4ocfafa/Fire Hydrant_LOD0__uh4ocfafa.gltf");
    //  std::string modelFile("C:\\Megascans/Pico/Roman Statue_tfraegpda/Roman Statue_LOD0__tfraegpda.gltf");
    //  std::string modelFile("C:\\Megascans/Pico/Cactus Pot_uenkeewfa/Cactus Pot_LOD0__uenkeewfa.gltf");

    // 




    //   std::string modelFile("../asset/gltf/Half Avocado_ujcxeblva_3D Asset/Half Avocado_LOD6__ujcxeblva.gltf");



    lmodel = document::model::Model::createFromGLTF(modelFile);
    if (!lmodel) {
        picoLog("Model <" + modelFile + "> document failed to load");
    }
    return lmodel;
}


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
    auto scene = std::make_shared<graphics::Scene>(graphics::SceneInit{gpuDevice, 1000100, 1000100, 1000, 10});
    state.scene = scene;

    // A Camera to look at the scene
    auto camera = scene->createCamera();
    camera->setViewport(1280.0f, 720.0f, true); // setting the viewport size, and yes adjust the aspect ratio
    camera->setOrientationFromRightUp({ 1.f, 0.f, 0.0f }, { 0.f, 1.f, 0.f });
    camera->setProjectionHeight(0.1f);
    camera->setFocal(0.1f);

    // The viewport managing the rendering of the scene from the camera
    auto viewport = std::make_shared<graphics::Viewport>(graphics::ViewportInit{ scene, gpuDevice, nullptr, camera->id() });



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

    // A point cloud draw factory
    auto pointCloudDrawFactory = std::make_shared<graphics::PointCloudDrawFactory>(gpuDevice);
    bool showPointCLoudAndTriangleSoup = false;
    if (showPointCLoudAndTriangleSoup) {

        // a draw from the pointcloud
        auto pcdrawable = scene->createDraw(pointCloudDrawFactory->createPointCloudDraw(gpuDevice, pointCloud));

        // A triangel soup draw factory
        auto triangleSoupDrawFactory = std::make_shared<graphics::TriangleSoupDrawFactory>(gpuDevice);

        // a draw from the trianglesoup
        auto tsdrawable = scene->createDraw(triangleSoupDrawFactory->createTriangleSoupDraw(gpuDevice, triangleSoup));

        // Some items unique instaces of the draw and the specified nodes
        auto pcitem = scene->createItem(node0, pcdrawable);

        auto tsitem = scene->createItem(enode, tsdrawable);

        auto pcitem2 = scene->createItem(cnode, pcdrawable);

        auto tsitem2 = scene->createItem(dnode, tsdrawable);
    }

    bool showModel = true;
    if (showModel)
    {
        state.models.rootNodeID = node0.id();
        auto modelItemIDs = generateModel(loadModel(), gpuDevice, state.scene, state.models.rootNodeID);
        if (modelItemIDs.size()) {
            state.models.modelItemID = modelItemIDs[0];
        }

    }
    
    // A Primitive draw factory
    auto primitiveDrawFactory = std::make_unique<graphics::PrimitiveDrawFactory>(gpuDevice);

    // a Primitive draw
    int numPrimitiveDraws = 10;
    graphics::Draws primitiveDraws(numPrimitiveDraws);
    for (int i = 0; i < numPrimitiveDraws; ++i) {
        float t = sin(core::pi()*float(i) / float(numPrimitiveDraws));
        core::vec3 size = { 1.0f + 2.0f * t, 2.0f + 3.0f * t, 0.7f + 2.0f * t };
        primitiveDraws[i] = scene->createDraw(primitiveDrawFactory->createPrimitive(gpuDevice, { {size} }));

    }

    // And many primitive items using the on of the primitive_draws
    std::vector<graphics::NodeID> prim_nodes;
    {
        int width = 0;
        float space = 4.0f;
        float pos_offset = width / 2 * space;
        for (int i = 0; i < width * width; ++i) {
            float t = acos(-1.0f) * i / float(width * width);
            auto p_node = scene->createNode(
                core::translation_rotation(
                    core::vec3(-space * (i % width) + pos_offset, -1.0f, space * (i / width) - pos_offset),
                    core::rotor3::make_from_x_to_dir(core::vec3(cos(t), 0, sin(t)))
                ),
                rnode.id());

            auto p_item = scene->createItem(p_node, primitiveDraws[i % numPrimitiveDraws]);
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


    // Create gizmos to draw the node transform and item tree
    auto [gznode_tree, gzitem_tree] = graphics::GizmoDraw_createSceneGizmos(scene, gpuDevice);

    // Dashboard
    auto dashboard_item = graphics::DashboardDraw_createSceneWidgets(scene, gpuDevice);

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
            scene->_nodes.editNodeTransform(rnode.id(), [t] (core::mat4x3& rts) -> bool {
                core::rotor3 rotor = core::rotor3::make_from_x_to_dir(core::vec3(cos(-t), 0.0f, sin(-t)));
                core::rotation(rts, rotor);
                return true;
            });
            scene->_nodes.editNodeTransform(bnode.id(), [t](core::mat4x3& rts) -> bool {
                return true;
            });
            scene->_nodes.editNodeTransform(cnode.id(), [t](core::mat4x3& rts) -> bool {
                core::rotor3 rotor = core::rotor3::make_from_x_to_dir(core::vec3(cos(0.2 * t), 0.0f, sin(0.2 * t)));
                core::rotation(rts, rotor);
                return true;
            });
            scene->_nodes.editNodeTransform(dnode.id(), [t](core::mat4x3& rts) -> bool {
                core::rotor3 rotor = core::rotor3::make_from_x_to_dir(core::vec3(cos(0.5 * t), sin(0.5 * t), 0.0f));
                core::rotation(rts, rotor);
                return true;
            });

            for (auto prim_node : prim_nodes) {
                scene->_nodes.editNodeTransform(prim_node, [t](core::mat4x3& rts) -> bool {
                    core::rotor3 rotor = core::rotor3::make_from_x_to_dir(core::vec3((float)cos(0.005 * t), 0.0f, (float)sin(0.005 * t)));
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

        if (e.state && e.key == uix::KEY_N) {
            gznode_tree.toggleVisible();
        }
        if (e.state && e.key == uix::KEY_B) {
            gzitem_tree.toggleVisible();
        }
        if (e.state && e.key == uix::KEY_M) {
            dashboard_item.toggleVisible();
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
                if (pointCloudDrawFactory) {
                    if (editPointCloudSize) {
                        auto v = pointCloudDrawFactory->getUniforms().getSpriteSize();
                        v -= e.delta.y * 0.01f;
                        pointCloudDrawFactory->editUniforms().setSpriteSize(v);
                    }
                    if (editPointCloudPerspectiveSpriteX) {
                        auto v = pointCloudDrawFactory->getUniforms().getPerspectiveSprite();
                        v = e.pos.x / ((float)window->width());
                        pointCloudDrawFactory->editUniforms().setPerspectiveSprite(v);
                    }
                    if (editPointCloudPerspectiveDepth) {
                        auto v = pointCloudDrawFactory->getUniforms().getPerspectiveDepth();
                        v -= e.delta.y * 0.01f;
                        pointCloudDrawFactory->editUniforms().setPerspectiveDepth(v);
                    }
                    if (editPointCloudShowPerspectiveDepth) {
                        auto v = pointCloudDrawFactory->getUniforms().getShowPerspectiveDepthPlane();
                        v = e.pos.x / ((float)window->width());
                        pointCloudDrawFactory->editUniforms().setShowPerspectiveDepthPlane(v);
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
