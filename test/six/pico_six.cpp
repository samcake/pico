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

#include <pico/content/PointCloud.h>
#include <pico/content/Mesh.h>

#include <vector>

//--------------------------------------------------------------------------------------

pico::PointCloudPointer createPointCloud(const std::string& filepath) {
    return pico::PointCloud::createFromPLY(filepath);
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

    // First a device, aka the gpu api used by pico
    pico::DeviceInit deviceInit {};
    auto gpuDevice = pico::Device::createDevice(deviceInit);


    // Content creation
    float doAnimate = 1.0f;

    // Some content, why not a pointcloud ?
    auto pointCloud = createPointCloud(cloudPointFile);

    // Step 1, create a Mesh from the point cloud data

    // Declare the vertex format == PointCloud::Point
    pico::Attribs<3> attribs{ {{ pico::AttribSemantic::A, pico::AttribFormat::VEC3, 0 }, { pico::AttribSemantic::B, pico::AttribFormat::VEC3, 0 }, {pico::AttribSemantic::C, pico::AttribFormat::CVEC4, 0 }} };
    pico::AttribBufferViews<1> bufferViews{ {0} };
    auto vertexFormat = pico::StreamLayout::build(attribs, bufferViews);

    // Create the Mesh for real
    pico::MeshPointer mesh = pico::Mesh::createFromPointArray(vertexFormat, (uint32_t)pointCloud->_points.size(), (const uint8_t*)pointCloud->_points.data());

    // Let's allocate buffer to hold the point cloud mesh
    pico::BufferInit vertexBufferInit{};
    vertexBufferInit.usage = pico::ResourceUsage::VERTEX_BUFFER;
    vertexBufferInit.hostVisible = true;
    vertexBufferInit.bufferSize = mesh->_vertexBuffers._buffers[0]->getSize();
    vertexBufferInit.vertexStride = mesh->_vertexBuffers._streamLayout.evalBufferViewByteStride(0);

    auto vertexBuffer = gpuDevice->createBuffer(vertexBufferInit);
    memcpy(vertexBuffer->_cpuMappedAddress, mesh->_vertexBuffers._buffers[0]->_data.data(), vertexBufferInit.bufferSize);

    auto numVertices = mesh->getNumVertices();

    // Let's describe the pipeline Descriptors layout
    pico::DescriptorLayouts descriptorLayouts {
        { pico::DescriptorType::UNIFORM_BUFFER, pico::ShaderStage::VERTEX, 0, 1}
    };

    pico::DescriptorSetLayoutInit descriptorSetLayoutInit{ descriptorLayouts };
    auto descriptorSetLayout = gpuDevice->createDescriptorSetLayout(descriptorSetLayoutInit);

    // And a Pipeline
    pico::ShaderInit vertexShaderInit{ pico::ShaderType::VERTEX, "mainVertex", "./vertex.hlsl" };
    pico::ShaderPointer vertexShader = gpuDevice->createShader(vertexShaderInit);

    pico::ShaderInit pixelShaderInit{ pico::ShaderType::PIXEL, "mainPixel", "./pixel.hlsl" };
    pico::ShaderPointer pixelShader = gpuDevice->createShader(pixelShaderInit);

    pico::ProgramInit programInit{ vertexShader, pixelShader };
    pico::ShaderPointer programShader = gpuDevice->createProgram(programInit);

    pico::PipelineStateInit pipelineInit{
        programShader,
        mesh->_vertexBuffers._streamLayout,
        pico::PrimitiveTopology::POINT,
        descriptorSetLayout
    };
    pico::PipelineStatePointer pipeline = gpuDevice->createPipelineState(pipelineInit);

    // It s time to create a descriptorSet that matches the expected pipeline descriptor set
    // then we will assign a uniform buffer in it
    pico::DescriptorSetInit descriptorSetInit{
        descriptorSetLayout
    };
    auto descriptorSet = gpuDevice->createDescriptorSet(descriptorSetInit);

    // Scene sphere:
    auto meshHalfSize = (mesh->_maxPos - mesh->_minPos);
    auto meshHalfDiag = sqrt(core::dot(meshHalfSize, meshHalfSize));
    core::vec4 sceneSphere(mesh->_midPos, meshHalfDiag);

    // A Camera to look at the scene
    auto camera = std::make_shared<pico::Camera>();
    camera->setViewport(1280.0f, 720.0f, true); // setting the viewport size, and yes adjust the aspect ratio
    camera->setOrientationFromRightUp({ 1.f, 0.f, 0.0f },{ 0.f, 1.f, 0.f });
    camera->zoomTo(sceneSphere);


    // Let s allocate a gpu buffer managed by the Camera
    camera->allocateGPUData(gpuDevice);

    // Assign the Camera UBO just created as the resource of the descriptorSet
    // auto descriptorObjects = descriptorSet->buildDescriptorObjects();
    pico::DescriptorObject uboDescriptorObject;
    uboDescriptorObject._uniformBuffers.push_back( camera->getGPUBuffer() );
    pico::DescriptorObjects descriptorObjects = {
        uboDescriptorObject,
    };
    gpuDevice->updateDescriptorSet(descriptorSet, descriptorObjects);
  
    // Renderer creation

    // And now a render callback where we describe the rendering sequence
    pico::RenderCallback renderCallback = [&](const pico::CameraPointer& camera, const pico::SwapchainPointer& swapchain, const pico::DevicePointer& device, const pico::BatchPointer& batch) {
        static float time = 0.0f;
        time += 1.0f / 60.0f;
        float intPart;
        float timeNorm = modf(time, &intPart);

        auto currentIndex = swapchain->currentIndex();

        if (doAnimate) {
            camera->setFocal(0.15f + 0.1f * sinf( 0.1f * time));
        }

        camera->updateGPUData();

        batch->begin(currentIndex);

        batch->resourceBarrierTransition(
            pico::ResourceBarrierFlag::NONE,
            pico::ResourceState::PRESENT,
            pico::ResourceState::RENDER_TARGET,
            swapchain, currentIndex, -1);

        core::vec4 clearColor(14.f/255.f, 14.f / 255.f, 14.f / 255.f, 1.f);
        batch->clear(swapchain, currentIndex, clearColor);

        batch->beginPass(swapchain, currentIndex);

        batch->setPipeline(pipeline);

        batch->bindVertexBuffers(1, &vertexBuffer);

        batch->setViewport(camera->getViewportRect());
        batch->setScissor(camera->getViewportRect());

        batch->bindDescriptorSet(descriptorSet);

        batch->draw(numVertices, 0);

        batch->endPass();

        batch->resourceBarrierTransition(
            pico::ResourceBarrierFlag::NONE,
            pico::ResourceState::RENDER_TARGET,
            pico::ResourceState::PRESENT,
            swapchain, currentIndex, -1);

        batch->end();

        device->executeBatch(batch);

        device->presentSwapchain(swapchain);
    };


    // Next, a renderer built on this device and callback
    auto renderer = std::make_shared<pico::Renderer>(gpuDevice, renderCallback);


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
    windowHandler->_onPaintDelegate = ([swapchain, renderer, camera](const pico::PaintEvent& e) {
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
        renderer->render(camera, swapchain);
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
            camera->zoomTo(sceneSphere);
        }

        if (e.state && e.key == pico::KEY_2) {
            // look lateral
            camera->setOrientationFromRightUp({ 0.f, 0.f, -1.f }, { 0.f, 1.f, 0.0f });
            camera->zoomTo(sceneSphere);
        }

        if (e.state && e.key == pico::KEY_3) {
            // look down
            camera->setOrientationFromRightUp({ 1.f, 0.f, 0.f }, { 0.f, 0.f, -1.f });
            camera->zoomTo(sceneSphere);
        }

        if (e.state && e.key == pico::KEY_4) {
            // look 3/4 down
            camera->setOrientationFromRightUp({ 1.f, 0.f, -1.f }, { 0.f, 1.f, -1.0f });
            camera->zoomTo(sceneSphere);
        }
    };

    windowHandler->_onMouseDelegate = [&](const pico::MouseEvent& e) {
        if (e.state & pico::MOUSE_MOVE) {
            if (e.state & pico::MOUSE_RBUTTON) {
                float orbitScale = 0.01f;
                camera->orbit(sceneSphere.w, e.delta.x * orbitScale, -e.delta.y * orbitScale);
            }
            if (e.state & pico::MOUSE_MBUTTON) {
                float panScale = sceneSphere.w * 0.001f;
                camera->pan(-e.delta.x* panScale, e.delta.y * panScale);
            }
        } else if (e.state & pico::MOUSE_WHEEL) {
            if (e.state & pico::MOUSE_CONTROL) {
                float zoomScale = 0.1f;
                camera->setFocal( camera->getFocal() * (1.0f +  e.wheel * zoomScale));
            } else {
                float dollyScale = 0.05f;
                sceneSphere.w *= (1.0f + e.wheel * dollyScale);
                camera->zoomTo(sceneSphere);
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
