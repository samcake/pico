// pico_ray.cpp 
//
// Sam Gateau - December 2021
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

#include <document/Model.h>

#include <graphics/gpu/Device.h>
#include <graphics/gpu/Resource.h>
#include <graphics/gpu/Swapchain.h>

#include <graphics/render/Camera.h>
#include <graphics/render/Viewport.h>

#include <graphics/drawables/GizmoDrawable.h>
#include <graphics/drawables/PrimitiveDrawable.h>
#include <graphics/drawables/DashboardDrawable.h>

#include <graphics/drawables/ModelDrawable.h>
#include <graphics/drawables/PostSceneDrawable.h>

#include <uix/Window.h>
#include <uix/Imgui.h>
#include <uix/CameraController.h>


#include <vector>


struct AppState {

    graphics::ModelDrawableFactoryPointer _modelDrawableFactory;
    graphics::ModelDrawableUniformsPointer _modelDrawableParams;

    graphics::PostSceneDrawableFactoryPointer _postSceneDrawableFactory;
    graphics::PostSceneDrawableUniformsPointer _postSceneDrawableParams;

    graphics::ScenePointer scene;
    struct {
        graphics::Node rootNode;
        graphics::ItemID modelItemID;
    } models;


    core::vec3 _modelInsertOffset;

};

AppState state;

//--------------------------------------------------------------------------------------
// pico ray:
//  Explore the ray tracing capcbilities of gpu!
//--------------------------------------------------------------------------------------



graphics::NodeIDs generateModel(document::ModelPointer lmodel, graphics::DevicePointer& gpuDevice, graphics::ScenePointer& scene, graphics::Node& root) {

    if (!state._postSceneDrawableFactory) {
        state._postSceneDrawableFactory = std::make_shared<graphics::PostSceneDrawableFactory>();
        state._postSceneDrawableFactory->allocateGPUShared(gpuDevice);
        state._postSceneDrawableParams = state._postSceneDrawableFactory->getUniformsPtr();
    }

    if (!state._modelDrawableFactory) {
        state._modelDrawableFactory = std::make_shared<graphics::ModelDrawableFactory>();
        state._modelDrawableFactory->allocateGPUShared(gpuDevice);
        state._modelDrawableParams = state._modelDrawableFactory->getUniformsPtr();
    }

    auto modelDrawablePtr = state._modelDrawableFactory->createModel(gpuDevice, lmodel);
    state._modelDrawableFactory->allocateDrawcallObject(gpuDevice, scene, *modelDrawablePtr);

    auto postProcessDrawablePtr = state._postSceneDrawableFactory->createDrawable(gpuDevice, modelDrawablePtr->_geometry);
    state._postSceneDrawableFactory->allocateDrawcallObject(gpuDevice, scene, *postProcessDrawablePtr);

    

    graphics::ItemIDs modelItemIDs;

    modelItemIDs = state._modelDrawableFactory->createModelParts(root.id(), scene, *modelDrawablePtr);

    // let's offset the root to not overlap on previous model
    if (modelItemIDs.size()) {
        auto modelRootNodeId = scene->getItem(modelItemIDs[0]).getNodeID();

        auto modelBound = modelDrawablePtr->getBound();
        auto minCorner = modelBound.minPos();
        auto maxCorner = modelBound.maxPos();
        auto boundCenter = modelBound.center;

        auto modelOffset = modelBound.half_size * core::vec3(1.0, 0.0, 1.0) * (1.0 + 0.1);
        auto modelPos = state._modelInsertOffset + modelOffset;
        modelPos.y += modelBound.half_size.y;


        modelPos = modelPos - boundCenter;

        scene->_nodes.editTransform(modelRootNodeId, [modelPos](core::mat4x3& rts) -> bool {
            core::translation(rts, modelPos);
            return true;
            });

        state._modelInsertOffset = state._modelInsertOffset + modelOffset * 2.0;
    }

    state._postSceneDrawableFactory->allocateDrawcallObject(gpuDevice, scene, *postProcessDrawablePtr);
    auto ppDrawable = scene->createDrawable(*postProcessDrawablePtr);
    auto pcitem = scene->createItem(root, ppDrawable);


    return modelItemIDs;
}

document::ModelPointer loadModel() {
    document::ModelPointer lmodel;
    //  std::string modelFile("../asset/gltf/toycar/toycar.gltf");
    //   std::string modelFile("../asset/gltf/AntiqueCamera.gltf");
    //   std::string modelFile("../asset/gltf/Sponza.gltf");
    //  std::string modelFile("../asset/gltf/WaterBottle/WaterBottle.gltf");
     std::string modelFile("../asset/gltf/Lantern/lantern.gltf");
    //  std::string modelFile("../asset/gltf/buggy.gltf");
    //   std::string modelFile("../asset/gltf/VC.gltf");
    //  std::string modelFile("../asset/gltf/duck.gltf");
    // std::string modelFile("../asset/gltf/OrientationTest.gltf");
    // std::string modelFile("../asset/gltf/DamagedHelmet/DamagedHelmet.gltf");
    // std::string modelFile("../asset/gltf/DamagedHelmet/DamagedHelmet-embedded.gltf");

    //  std::string modelFile("../asset/gltf/Rusted Metal Barrel_teufceuda_Metal/Rusted Metal Barrel_LOD2__teufceuda.gltf");
    //  std::string modelFile("../asset/gltf/Castle Parapet Wall_sbxuw_3D Asset/Castle Parapet Wall_LOD0__sbxuw.gltf");
    // std::string modelFile("../asset/gltf/Half Avocado_ujcxeblva_3D Asset/Half Avocado_LOD0__ujcxeblva.gltf");

   // std::string modelFile("C:\\Megascans/Pico/Banana_vfendgyiw/Banana_LOD0__vfendgyiw.gltf");
    //std::string modelFile("C:\\Megascans/Pico/Nordic Beach Rock_uknoehp/Nordic Beach Rock_LOD0__uknoehp.gltf");

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

    return lmodel;
}

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
    state.scene = std::make_shared<graphics::Scene>();
    state.scene->_items.resizeBuffers(gpuDevice, 250000);
    state.scene->_nodes.resizeBuffers(gpuDevice, 250000);
    state.scene->_drawables.resizeBuffers(gpuDevice, 250000);

    // A Camera to look at the scene
    auto camera = std::make_shared<graphics::Camera>();
    camera->setViewport(1280.0f, 720.0f, true); // setting the viewport size, and yes adjust the aspect ratio
    camera->setOrientationFromRightUp({ 1.f, 0.f, 0.0f }, { 0.f, 1.f, 0.f });
    camera->setProjectionHeight(0.1f);
    camera->setFocal(0.1f);

    // The view managing the rendering of the scene from the camera
    auto viewport = std::make_shared<graphics::Viewport>(state.scene, camera, gpuDevice,
        uix::Imgui::standardPostSceneRenderCallback);

    // A gizmo drawable factory
    auto gizmoDrawableFactory = std::make_shared<graphics::GizmoDrawableFactory>();
    gizmoDrawableFactory->allocateGPUShared(gpuDevice);

    // a gizmo drawable to draw the transforms
    auto gzdrawable_node = state.scene->createDrawable(*gizmoDrawableFactory->createNodeGizmo(gpuDevice));
    gizmoDrawableFactory->allocateDrawcallObject(gpuDevice, state.scene, gzdrawable_node.as<graphics::NodeGizmo>());
    gzdrawable_node.as<graphics::NodeGizmo>().nodes.resize(state.scene->_nodes._nodes_buffer->numElements());
    auto gzitem_node = state.scene->createItem(graphics::Node::null, gzdrawable_node);
    gzitem_node.setVisible(false);


    auto gzdrawable_item = state.scene->createDrawable(*gizmoDrawableFactory->createItemGizmo(gpuDevice));
    gizmoDrawableFactory->allocateDrawcallObject(gpuDevice, state.scene, gzdrawable_item.as<graphics::ItemGizmo>());
    gzdrawable_item.as<graphics::ItemGizmo>().items.resize(state.scene->_items._items_buffer->numElements());
    auto gzitem_item = state.scene->createItem(graphics::Node::null, gzdrawable_item);
    gzitem_item.setVisible(false);


    // A dashboard factory and drawable to represent some debug data
    auto dashboardDrawableFactory = std::make_shared<graphics::DashboardDrawableFactory>();
    dashboardDrawableFactory->allocateGPUShared(gpuDevice);

    // a dashboard
    auto dashboard_drawable = state.scene->createDrawable(*dashboardDrawableFactory->createDrawable(gpuDevice));
    dashboardDrawableFactory->allocateDrawcallObject(gpuDevice, state.scene, dashboard_drawable.as<graphics::DashboardDrawable>());

    auto dashboard_item = state.scene->createItem(graphics::Node::null, dashboard_drawable);

    // Some nodes to layout the scene and animate objects
    state.models.rootNode = state.scene->createNode(core::mat4x3(), -1);

    auto modelItemIDs = generateModel(loadModel(), gpuDevice, state.scene, state.models.rootNode);
    if (modelItemIDs.size()) {
         state.models.modelItemID = modelItemIDs[0];
    }

    state.scene->_nodes.updateTransforms();


    state.scene->updateBounds();
    core::vec4 sceneSphere = state.scene->getBounds().toSphere();
    
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
    uix::WindowInit windowInit { windowHandler, "Pico Eye" };
    auto window = uix::Window::createWindow(windowInit);

    // Setup Dear ImGui context with the gpuDevice and the brand new window
    uix::Imgui::create();
    uix::Imgui::setup(window, gpuDevice);

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
            std::string title = std::string("Pico Eye: ") + std::to_string((uint32_t) frameSample.beginRate())
                              + std::string("Hz ") + std::to_string(0.001f * frameSample._frameDuration.count()) + std::string("ms")
                              + (camera->isOrtho()
                                    ? (std::string(" ortho:") + std::to_string((int)(1000.0f * camera->getOrthoHeight())) + std::string("mm"))
                                    : (std::string(" fov:") + std::to_string((int)(camera->getFovDeg())) + std::string("deg")) );
            window->setTitle(title);
        }
        auto t = (currentSample._frameNum / 90.0f);
        auto ts = acos(-1.0f) * (currentSample._frameNum / 300.0f);

        if (doAnimate) {
        }
        uix::Imgui::newFrame();


        if (ImGui::Begin("Eye")) {
            if (state._modelDrawableParams) {
                auto& params = *state._modelDrawableParams.get();

                static const char* displayedNames[] = {
                    "albedo", "normal", "surface normal", "normal map", "rao map", "grey"
                };
                if (ImGui::BeginCombo("Displayed Color", displayedNames[params.displayedColor])) {
                    for (int n = 0; n < IM_ARRAYSIZE(displayedNames); n++) {
                        bool is_selected = (params.displayedColor == n); // You can store your selection however you want, outside or inside your objects
                        if (ImGui::Selectable(displayedNames[n], is_selected)) {
                            params.displayedColor = n;
                        }
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                    }
                    ImGui::EndCombo();
                }

                ImGui::Separator();

                ImGui::Checkbox("Light shading", &params.lightShading);
            }
        }
        ImGui::End();
    /*    if (ImGui::Begin("Scene")) {
            static char buffer[512] = "";
            int buffer_size = 512;
            if (ImGui::InputText("Open...", buffer, buffer_size)) {
                std::clog << "? " << buffer << std::endl;
            }

        }
        ImGui::End();
        */

        state.scene->_items.syncBuffer();
        state.scene->_nodes.updateTransforms();
        camControl->update(std::chrono::duration_cast<std::chrono::microseconds>(frameSample._frameDuration));

        // Render!
        viewport->present(swapchain);
    });

    // On resize deal with it
    windowHandler->_onResizeDelegate = [&](const uix::ResizeEvent& e) {
        // only resize the swapchain when we re done with the resize
      //  if (e.over) {
            gpuDevice->resizeSwapchain(swapchain, e.width, e.height);
      //  }

        camControl->onResize(e);

        if (e.over) {
            camControl->onResize(e);
        }
    };

    windowHandler->_onKeyboardDelegate = [&](const uix::KeyboardEvent& e) {
        if (e.state && e.key == uix::KEY_SPACE) {
            doAnimate = (doAnimate == 0.f ? 1.0f : 0.0f);
        }

        if (e.state && e.key == uix::KEY_N) {
            gzitem_node.setVisible(!gzitem_node.isVisible());
        }
        if (e.state && e.key == uix::KEY_B) {
            gzitem_item.setVisible(!gzitem_item.isVisible());
        }

        if (e.state && e.key == uix::KEY_DELETE) {

            if (state.models.modelItemID) {
                gpuDevice->flush();
                state.scene->deleteItem(state.models.modelItemID);
                state.models.modelItemID = graphics::INVALID_NODE_ID;
            }
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
        
        // Default app mode case appMode == 0
        if (camControl->onMouse(e)) {
            return;
        }
    };
 
    windowHandler->_onDropFilesDelegate = [&](const uix::DropFilesEvent& e) {
        
        if (e.fileUrls.size()) {
            e.fileUrls;
            // let's try to load the model

            document::ModelPointer lmodel = document::model::Model::createFromGLTF(e.fileUrls[0]);
            if (lmodel) {
                auto modelItemIDs = generateModel(lmodel, gpuDevice, state.scene, state.models.rootNode);
            }
        }
        return;
    };

    // Render Loop 
    bool keepOnGoing = true;
    while (keepOnGoing) {
        keepOnGoing = window->messagePump();
    }

    uix::Imgui::destroy();
    core::api::destroy();

    return 0;
}