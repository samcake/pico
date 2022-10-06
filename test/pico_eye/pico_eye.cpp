// pico_eye.cpp 
//
// Sam Gateau - August 2021
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
#include <core/Log.h>

#include <document/Model.h>

#include <graphics/gpu/Device.h>
#include <graphics/gpu/Resource.h>
#include <graphics/gpu/Swapchain.h>

#include <graphics/render/Camera.h>
#include <graphics/render/Viewport.h>

#include <graphics/drawables/SkyDrawable.h>
#include <graphics/drawables/GizmoDrawable.h>
#include <graphics/drawables/PrimitiveDrawable.h>
#include <graphics/drawables/DashboardDrawable.h>

#include <graphics/drawables/ModelDrawable.h>

#include <uix/Window.h>
#include <uix/Imgui.h>
#include <uix/CameraController.h>


#include <vector>


struct AppState {

    graphics::ModelDrawableFactoryPointer _modelDrawableFactory;
    graphics::ModelDrawableUniformsPointer _modelDrawableParams;


    graphics::ScenePointer scene;
    struct {
        graphics::Node rootNode;
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

//--------------------------------------------------------------------------------------
// pico eye:
//  Explore the definition of a document::model created from loading a gltf file
//  and its drawable counterpart, MOdelDrawable
//--------------------------------------------------------------------------------------



graphics::NodeIDs generateModel(document::ModelPointer lmodel, graphics::DevicePointer& gpuDevice, graphics::ScenePointer& scene, graphics::Node& root) {

    if (!state._modelDrawableFactory) {
        state._modelDrawableFactory = std::make_shared<graphics::ModelDrawableFactory>();
        state._modelDrawableFactory->allocateGPUShared(gpuDevice);
        state._modelDrawableParams = state._modelDrawableFactory->getUniformsPtr();
    }

    graphics::ItemIDs modelItemIDs;
    if (lmodel) {
        auto modelDrawablePtr = state._modelDrawableFactory->createModel(gpuDevice, lmodel);
        state._modelDrawableFactory->allocateDrawcallObject(gpuDevice, scene, *modelDrawablePtr);


        modelItemIDs = state._modelDrawableFactory->createModelParts(root.id(), scene, *modelDrawablePtr);

        // let's offset the root to not overlap on previous model
        if (modelItemIDs.size()) {
            auto modelRootNodeId = scene->getItem(modelItemIDs[0]).nodeID();

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
    } else {
        picoLog("Model document is empty");
    }

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
    state.scene = std::make_shared<graphics::Scene>(graphics::SceneInit{ gpuDevice, 10000, 10000, 10000, 10 });

    // A Camera to look at the scene
    auto camera = state.scene->createCamera();
    camera->setViewport(1280.0f, 720.0f, true); // setting the viewport size, and yes adjust the aspect ratio
    camera->setOrientationFromRightUp({ 1.f, 0.f, 0.0f }, { 0.f, 1.f, 0.f });
    camera->setProjectionHeight(0.1f);
    camera->setFocal(0.1f);

    state.cams._current = camera->id();

    // A sky drawable factory
    auto skyDrawableFactory = state.scene->_skyFactory;

    // The view managing the rendering of the scene from the camera
    graphics::ViewportInit viewportInit = { state.scene, gpuDevice, uix::Imgui::standardPostSceneRenderCallback };
    auto viewport = std::make_shared<graphics::Viewport>(viewportInit);

    // a sky drawable to draw the sky
    auto skyDrawable = state.scene->createDrawable(*skyDrawableFactory->createDrawable(gpuDevice));
    skyDrawableFactory->allocateDrawcallObject(gpuDevice, state.scene, skyDrawable.as<graphics::SkyDrawable>());
    auto skyitem = state.scene->createItem(graphics::Node::null, skyDrawable);
    skyitem.setVisible(true);


    // A gizmo drawable factory
    auto gizmoDrawableFactory = std::make_shared<graphics::GizmoDrawableFactory>();
    gizmoDrawableFactory->allocateGPUShared(gpuDevice);

    // a gizmo drawable to draw the transforms
    auto gzdrawable_node = state.scene->createDrawable(*gizmoDrawableFactory->createNodeGizmo(gpuDevice));
    gizmoDrawableFactory->allocateDrawcallObject(gpuDevice, state.scene, gzdrawable_node.as<graphics::NodeGizmo>());
    gzdrawable_node.as<graphics::NodeGizmo>().nodes.resize(state.scene->_nodes._nodes_buffer->numElements());
    state.tools.tree_node = state.scene->createItem(graphics::Node::null, gzdrawable_node);
    state.tools.tree_node.setVisible(false);


    auto gzdrawable_item = state.scene->createDrawable(*gizmoDrawableFactory->createItemGizmo(gpuDevice));
    gizmoDrawableFactory->allocateDrawcallObject(gpuDevice, state.scene, gzdrawable_item.as<graphics::ItemGizmo>());
    gzdrawable_item.as<graphics::ItemGizmo>().items.resize(state.scene->_items.getGPUBuffer()->numElements());
    state.tools.tree_item = state.scene->createItem(graphics::Node::null, gzdrawable_item);
    state.tools.tree_item.setVisible(false);


    // A dashboard factory and drawable to represent some debug data
    auto dashboardDrawableFactory = std::make_shared<graphics::DashboardDrawableFactory>();
    dashboardDrawableFactory->allocateGPUShared(gpuDevice);

    // a dashboard
    auto dashboard_drawable = state.scene->createDrawable(*dashboardDrawableFactory->createDrawable(gpuDevice));
    dashboardDrawableFactory->allocateDrawcallObject(gpuDevice, state.scene, dashboard_drawable.as<graphics::DashboardDrawable>());
    state.tools.dashboard = state.scene->createItem(graphics::Node::null, dashboard_drawable);
    state.tools.dashboard.setVisible(false);

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
            std::string title = std::string("Pico Eye: ") + std::to_string((uint32_t)frameSample.beginRate())
                + std::string("Hz ") + std::to_string(0.001f * frameSample._frameDuration.count()) + std::string("ms")
                + (camera->isOrtho()
                    ? (std::string(" ortho:") + std::to_string((int)(1000.0f * camera->getOrthoHeight())) + std::string("mm"))
                    : (std::string(" fov:") + std::to_string((int)(camera->getFovDeg())) + std::string("deg")));
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

            if (ImGui::CollapsingHeader("Sky")) {
                bool skyDebug = state.scene->_sky->isDebugEnabled();
                if (ImGui::Checkbox("Show debug scopes", &skyDebug))
                    state.scene->_sky->setDebugEnabled(skyDebug);

                const float c_rad_to_deg = 180.0 / acos(-1);
                auto sunDir = state.scene->_sky->getSunDir();
                auto sunAE = core::spherical_dir_to_azimuth_elevation(sunDir) * c_rad_to_deg;
                bool sunChanged = false;
                sunChanged |= ImGui::SliderFloat("Sun Azimuth", &sunAE.x, -180, 180, "%.0f");
                sunChanged |= ImGui::SliderFloat("Sun Elevation", &sunAE.y, -90, 90, "%.0f");
                if (sunChanged) {
                    sunAE = core::scale(sunAE, (1.0 / c_rad_to_deg));
                    sunDir = core::spherical_dir_from_azimuth_elevation(sunAE.x, sunAE.y);
                    state.scene->_sky->setSunDir(sunDir);
                }

                float altitude = state.scene->_sky->getStageAltitude() * 0.001;
                if (ImGui::SliderFloat("Stage Altitude", &altitude, 0.001, 100000, "%.3f km", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat)) {
                    state.scene->_sky->setStageAltitude(altitude * 1000.0);
                }

                auto simDim = state.scene->_sky->getSimDim();
                bool simDimChanged = false;
                simDimChanged |= ImGui::SliderInt("Sim num ray samples", &simDim.x, 1, 64);
                simDimChanged |= ImGui::SliderInt("Sim num light samples", &simDim.y, 1, 64);
                simDimChanged |= ImGui::SliderInt("Diffuse Env sample count", &simDim.z, 8, simDim.w);
                if (simDimChanged) {
                    state.scene->_sky->setSimDim(simDim);
                }
            }
        }
        ImGui::End();


        camControl->update(std::chrono::duration_cast<std::chrono::microseconds>(frameSample._frameDuration));

        // Render!
        viewport->present(swapchain);
    });

    // On resize deal with it
    windowHandler->_onResizeDelegate = [&](const uix::ResizeEvent& e) {
        // only resize the swapchain when we re done with the resize
      //  if (e.done) {
            gpuDevice->resizeSwapchain(swapchain, e.width, e.height);
      //  }

        camControl->onResize(e);

        if (e.done) {
            camControl->onResize(e);
        }
    };
    windowHandler->_onResizeDelegate({ window->width(), window->height(), true });

    windowHandler->_onKeyboardDelegate = [&](const uix::KeyboardEvent& e) {
        if (e.state && e.key == uix::KEY_SPACE) {
            doAnimate = (doAnimate == 0.f ? 1.0f : 0.0f);
        }

        if (e.state && e.key == uix::KEY_N) {
            state.tools.tree_node.toggleVisible();
        }
        if (e.state && e.key == uix::KEY_B) {
            state.tools.tree_item.toggleVisible();
        }
        if (e.state && e.key == uix::KEY_M) {
            state.tools.dashboard.toggleVisible();
        }

        if (e.state && e.key == uix::KEY_DELETE) {

            if (state.models.modelItemID) {
                gpuDevice->flush();
                state.scene->deleteItem(state.models.modelItemID);
                state.models.modelItemID = graphics::INVALID_NODE_ID;
            }
        }

        if (e.state && e.key == uix::KEY_O) {
            camControl->_orbitOnMouseMoveEnabled = !camControl->_orbitOnMouseMoveEnabled;
        }

        bool zoomToScene = false;
        if (e.state && e.key == uix::KEY_1) {
            // look side
            camera->setOrientationFromRightUp({ 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.0f });
            zoomToScene = true;
        }
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
