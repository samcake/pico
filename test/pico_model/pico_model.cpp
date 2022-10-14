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

#include <document/Model.h>

#include <graphics/gpu/Device.h>
#include <graphics/gpu/Resource.h>
#include <graphics/gpu/Swapchain.h>

#include <graphics/render/Camera.h>
#include <graphics/render/Viewport.h>

#include <graphics/drawables/GizmoDraw.h>
#include <graphics/drawables/PrimitiveDraw.h>
#include <graphics/drawables/ModelDraw.h>
#include <graphics/drawables/ModelDrawInspector.h>
#include <graphics/drawables/SkyDraw.h>

#include <uix/Window.h>
#include <uix/Imgui.h>
#include <uix/CameraController.h>


#include <vector>


struct AppState {

    uint32_t makeUVMapFrame = 0;
    uint32_t makeFilteredMapFrame = 0;

    graphics::ModelDrawFactoryPointer _modelDrawFactory;
    graphics::ModelDrawInspectorFactoryPointer _modelDrawInspectorFactory;
    graphics::ModelDrawInspectorUniformsPointer params;


    graphics::ScenePointer _scene;

    graphics::NodeID _modelRootNodeID;

    graphics::ItemID _modelItemID;

};

AppState state;

//--------------------------------------------------------------------------------------
// pico model:
//  Explore the definition of a document::model created from loading a gltf file
//  and its draw counterpart, MOdelDraw
//--------------------------------------------------------------------------------------

document::ModelPointer lmodel;


graphics::NodeIDs generateModel(graphics::DevicePointer& gpuDevice, graphics::ScenePointer& scene, graphics::CameraPointer& camera, graphics::NodeID nodeRoot, bool withInspector) {

 //  std::string modelFile("../asset/gltf/toycar/toycar.gltf");
 //   std::string modelFile("../asset/gltf/AntiqueCamera.gltf");
 //   std::string modelFile("../asset/gltf/Sponza.gltf");
  //  std::string modelFile("../asset/gltf/WaterBottle/WaterBottle.gltf");
  // std::string modelFile("../asset/gltf/Lantern/lantern.gltf");
  //  std::string modelFile("../asset/gltf/buggy.gltf");
   //   std::string modelFile("../asset/gltf/VC.gltf");
    //  std::string modelFile("../asset/gltf/duck.gltf");
   // std::string modelFile("../asset/gltf/OrientationTest.gltf");
   // std::string modelFile("../asset/gltf/DamagedHelmet/DamagedHelmet.gltf");
  // std::string modelFile("../asset/gltf/DamagedHelmet/DamagedHelmet-embedded.gltf");
    
   //  std::string modelFile("../asset/gltf/Rusted Metal Barrel_teufceuda_Metal/Rusted Metal Barrel_LOD2__teufceuda.gltf");
   //  std::string modelFile("../asset/gltf/Castle Parapet Wall_sbxuw_3D Asset/Castle Parapet Wall_LOD0__sbxuw.gltf");
   // std::string modelFile("../asset/gltf/Half Avocado_ujcxeblva_3D Asset/Half Avocado_LOD0__ujcxeblva.gltf");

   std::string modelFile("C:\\Megascans/Pico/Banana_vfendgyiw/Banana_LOD0__vfendgyiw.gltf");
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

    if (!state._modelDrawFactory) {
        state._modelDrawFactory = std::make_shared<graphics::ModelDrawFactory>();
        state._modelDrawFactory->allocateGPUShared(gpuDevice);
    }

    auto modelDrawPtr = state._modelDrawFactory->createModel(gpuDevice, lmodel);
    state._modelDrawFactory->allocateDrawcallObject(gpuDevice, scene, *modelDrawPtr);

    auto modelDraw = scene->createDraw(*modelDrawPtr);

    graphics::ItemIDs modelItemIDs;

    if (withInspector) {
        if (!state._modelDrawInspectorFactory) {
            state._modelDrawInspectorFactory = std::make_shared<graphics::ModelDrawInspectorFactory>();
            state._modelDrawInspectorFactory->allocateGPUShared(gpuDevice);
            state.params = state._modelDrawInspectorFactory->getUniformsPtr();
            state.params->setFilterKernelTechnique(graphics::ModelDrawInspectorUniforms::FKT_IMAGE_SPACE);
        }
        auto modelDrawInspectorPtr = state._modelDrawInspectorFactory->createModel(gpuDevice, lmodel, modelDrawPtr);
        state._modelDrawInspectorFactory->allocateDrawcallObject(gpuDevice, scene, *modelDrawInspectorPtr);

        auto modelDrawInspector = scene->createDraw(*modelDrawInspectorPtr);

        modelItemIDs = state._modelDrawInspectorFactory->createModelParts(nodeRoot, scene, *modelDrawInspectorPtr);
    } else {
        modelItemIDs = state._modelDrawFactory->createModelParts(nodeRoot, scene, *modelDrawPtr);
    }

    return modelItemIDs;
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
    auto scene = state._scene = std::make_shared<graphics::Scene>(graphics::SceneInit{gpuDevice, 10000, 10000, 10000, 10});

    // A Camera to look at the scene
    auto camera = state._scene->createCamera();
    camera->setViewport(1280.0f, 720.0f, true); // setting the viewport size, and yes adjust the aspect ratio
    camera->setOrientationFromRightUp({ 1.f, 0.f, 0.0f }, { 0.f, 1.f, 0.f });
    camera->setProjectionHeight(0.1f);
    camera->setFocal(0.1f);

    // A sky draw factory
    auto skyDrawFactory = scene->_skyFactory;

    // The view managing the rendering of the scene
    graphics::ViewportInit viewportInit = { state._scene, gpuDevice, uix::Imgui::standardPostSceneRenderCallback };
    auto viewport = std::make_shared<graphics::Viewport>(viewportInit);

    // Create gizmos to draw the node transform and item tree
    auto [gznode_tree, gzitem_tree] = graphics::GizmoDraw_createSceneGizmos(state._scene, gpuDevice);

    // Some nodes to layout the scene and animate objects
    state._modelRootNodeID = scene->createNode(core::mat4x3(), -1).id();

    auto modelItemIDs = generateModel(gpuDevice, scene, camera, state._modelRootNodeID, true);
    if (modelItemIDs.size()) {
         state._modelItemID = modelItemIDs[0];
    }

    scene->_nodes.updateTransforms();


    scene->updateBounds();
    core::vec4 sceneSphere = scene->getBounds().toSphere();
    
    // Content creation
    float doAnimate = 0.0f;

    // For user input, we can use a CameraController which will provide a standard manipulation of the camera from keyboard and mouse
    auto camControl = std::make_shared< uix::CameraController >(camera);
    float cameraOrbitLength = camControl->zoomTo(sceneSphere);
    camera->setFar(cameraOrbitLength * 100.0f);


    // A UV space camera to look at the uv space
    auto uv_camera = scene->createCamera();
    uv_camera->setViewport(1280.0f, 720.0f, true); // setting the viewport size, and yes adjust the aspect ratio
    uv_camera->setOrientationFromRightUp({ 1.f, 0.f, 0.0f }, { 0.f, 1.f, 0.f });
    uv_camera->setOrthoHeight(1.0f);
    uv_camera->setOrtho(true);
    uv_camera->setEye(core::vec3(0.5, 0.5, 0));
    auto uv_camControl = std::make_shared< uix::CameraController >(uv_camera, true);


    // Presentation creation

    // We need a window where to present, let s use the graphics::Window for convenience
    // This could be any window, we just need the os handle to create the swapchain next.
    auto windowHandler = new uix::WindowHandlerDelegate();
    uix::WindowInit windowInit { windowHandler, "Pico Model" };
    auto window = uix::Window::createWindow(windowInit);

    // Setup Dear ImGui context with the gpuDevice and the brand new window
    uix::Imgui::create();
    uix::Imgui::setup(window, gpuDevice);

    camera->setViewport(window->width(), window->height(), true); // setting the viewport size, and yes adjust the aspect ratio
    uv_camera->setViewport(window->width(), window->height(), true);

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
            std::string title = std::string("Pico Model: ") + std::to_string((uint32_t) frameSample.beginRate())
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


        if (ImGui::Begin("UV Seams")) {
            if (state.params) {
                auto& params = *state.params.get();
                bool runFilter = false;


                static const char* displayedNames[] = {
                    "albedo", "normal", "surface normal", "normal map"
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


                ImGui::Checkbox("UV Space", &params.renderUVSpace);

                ImGui::Separator();

                ImGui::Checkbox("Render mesh", &params.render3DModel);
                ImGui::Checkbox("Render wireframe", &params.renderWireframe);
                ImGui::Checkbox("Render uvmesh points", &params.renderUVMeshPoints);
                ImGui::Checkbox("Render edge lines", &params.renderUVEdgeLines);

                ImGui::Separator();
                ImGui::Checkbox("Show uv grid", &params.showUVGrid);
                ImGui::Checkbox("Show uvmesh outside", &params.showUVMeshOutside);
                ImGui::Checkbox("Show uvmesh edges", &params.showUVMeshEdges);
                ImGui::Checkbox("Show uvmesh faces", &params.showUVMeshFaces);
                ImGui::Checkbox("Show uvmesh faces id", &params.showUVMeshFacesID);
                ImGui::Separator();

                ImGui::Checkbox("Light shading", &params.lightShading);

                ImGui::Checkbox("Linear sampler", &params.linearSampler);

                ImGui::Separator();

                ImGui::Checkbox("Render connectivity", &params.renderConnectivity);
                ImGui::BeginGroup();
                    ImGui::PushID("connectivity");
                    if (ImGui::Button("-")) {
                        params.numInspectedTriangles -= 1;
                        if (params.numInspectedTriangles < 1) params.numInspectedTriangles = 1;
                    }
                    ImGui::SameLine();
                    ImGui::SliderInt("Triangles", &params.numInspectedTriangles, 1, 32);
                    ImGui::SameLine();
                    if (ImGui::Button("+")) {
                        params.numInspectedTriangles += 1;
                        if (params.numInspectedTriangles > 32) params.numInspectedTriangles = 32;
                    }
                    ImGui::PopID();
                ImGui::EndGroup();

                ImGui::Separator();

                ImGui::Text("Inspected triangle");
                ImGui::BeginGroup();
                    ImGui::PushID("inspected");

                    if (ImGui::Button("-")) {
                        params.inspectedTriangle -= 1;
                        if (params.inspectedTriangle < -1) params.inspectedTriangle = -1;
                    }
                    ImGui::SameLine();

                    runFilter |= ImGui::SliderInt(std::to_string(params.numTriangles).c_str(), &params.inspectedTriangle, -1, params.numTriangles - 1);
                    ImGui::SameLine();
                    if (ImGui::Button("+")) {
                        params.inspectedTriangle += 1;
                        if (params.inspectedTriangle >= params.numTriangles) params.inspectedTriangle = params.numTriangles - 1;
                    }
                    ImGui::PopID();
                ImGui::EndGroup();
                ImGui::Separator();

                ImGui::Text("Inspected texel");
                ImGui::Checkbox("Render kernel samples", &params.renderKernelSamples);

                ImGui::PushID("inspected texel x");

                if (ImGui::Button("-")) {
                    params.inspectedTexelX -= 1;
                    if (params.inspectedTexelX < 0) params.inspectedTexelX = 0;
                }
                ImGui::SameLine();

                ImGui::SliderInt("Texel X", &params.inspectedTexelX, 0, params.mapWidth - 1);
                ImGui::SameLine();
                if (ImGui::Button("+")) {
                    params.inspectedTexelX += 1;
                    if (params.inspectedTexelX >= params.mapWidth) params.inspectedTexelX = params.mapWidth - 1;
                }
                ImGui::PopID();

                ImGui::PushID("inspected texel y");

                if (ImGui::Button("-")) {
                    params.inspectedTexelY -= 1;
                    if (params.inspectedTexelY < 0) params.inspectedTexelY = 0;
                }
                ImGui::SameLine();

                ImGui::SliderInt("Texel Y", &params.inspectedTexelY, 0, params.mapHeight - 1);

                ImGui::SameLine();
                if (ImGui::Button("+")) {
                    params.inspectedTexelY += 1;
                    if (params.inspectedTexelY >= params.mapHeight) params.inspectedTexelY = params.mapHeight - 1;
                }
                ImGui::PopID();

                ImGui::Separator();


                bool makeUVMeshMap = false;
                makeUVMeshMap |= ImGui::Button((std::string("Make uvmesh map ") + std::to_string(state.makeUVMapFrame)).c_str());
                makeUVMeshMap |= ImGui::Checkbox("With uvmesh edge lines pass", &params.uvmeshEdgeLinesPass);
                if (makeUVMeshMap) {
                    params.makeUVMeshMap = true;
                    state.makeUVMapFrame++;

                    // trigger a new computed filter
                    runFilter |= true;
                }
                ImGui::Separator();

                runFilter |= ImGui::Button((std::string("Make filtered map ") + std::to_string(state.makeFilteredMapFrame)).c_str());
                {
                    static const char* techniqueNames[] = {
                        "Image space", "3D space"
                    };
                    
                    if (ImGui::BeginCombo("Filter Technique", techniqueNames[params.filterKernelTechnique])) {
                        for (int n = 0; n < IM_ARRAYSIZE(techniqueNames); n++) {
                            bool is_selected = (params.filterKernelTechnique == n); // You can store your selection however you want, outside or inside your objects
                            if (ImGui::Selectable(techniqueNames[n], is_selected)) {
                                runFilter |= (params.filterKernelTechnique != n); // if inspected map change, regenerate the filtered texture
                                params.setFilterKernelTechnique(n);
                            }
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                        }
                        ImGui::EndCombo();
                    }

                    static const char* mapNames[] = {
                        "Albedo", "Normal"
                    };
                    if (ImGui::BeginCombo("Source Map", mapNames[params.inspectedMap])) {
                        for (int n = 0; n < IM_ARRAYSIZE(mapNames); n++) {
                            bool is_selected = (params.inspectedMap == n); // You can store your selection however you want, outside or inside your objects
                            if (ImGui::Selectable(mapNames[n], is_selected)) {
                                runFilter |= (params.inspectedMap != n); // if inspected map change, regenerate the filtered texture
                                params.inspectedMap = n;
                            }
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                        }
                        ImGui::EndCombo();
                    }
                    runFilter |= ImGui::Checkbox("Mask outside", &params.maskOutsideUV);

                    runFilter |= ImGui::SliderFloat("Kernel radius", &params.kernelRadius, 0.0f, params.kernelRadiusMax);
                }

                if (runFilter) {
                    params.makeComputedMap = true;
                    state.makeFilteredMapFrame++;
                }

                ImGui::Separator();

                // Blending slider to copare filtered texture against base
                ImGui::SliderFloat("Filtered Map", &params.colorMapBlend, 0.0f, 1.0f);

            }
        }
        ImGui::End();

        camControl->update(std::chrono::duration_cast<std::chrono::microseconds>(frameSample._frameDuration));
        uv_camControl->update(std::chrono::duration_cast<std::chrono::microseconds>(frameSample._frameDuration));
        if (state.params) {
            state.params->uvSpaceCenterX = uv_camera->getEye().x;
            state.params->uvSpaceCenterY = uv_camera->getEye().y;
            state.params->uvSpaceScale = uv_camera->getOrthoHeight() * 0.5f;
        }

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
        uv_camControl->onResize(e);
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

        if (e.state && e.key == uix::KEY_DELETE) {

            if (state._modelItemID) {
                gpuDevice->flush();
                state._scene->deleteItem(state._modelItemID);
                state._modelItemID = graphics::INVALID_NODE_ID;
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
        // UV space mode
        if (state.params && state.params->renderUVSpace) {
            (uv_camControl->onMouse(e)); {
                state.params->uvSpaceCenterX = uv_camera->getEye().x;
                state.params->uvSpaceCenterY = uv_camera->getEye().y;
                state.params->uvSpaceScale = uv_camera->getOrthoHeight() * 0.5f;

                return;
            }
            return;
        }

        // Default app mode case appMode == 0
        if (camControl->onMouse(e)) {
            return;
        }
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
