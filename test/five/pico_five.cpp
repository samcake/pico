// pico_five.cpp 
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

#include <pico/Window.h>

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

#include <vector>

//--------------------------------------------------------------------------------------

pico::PointCloudPointer createPointCloud(const std::string& filepath) {
    return pico::PointCloud::createFromPLY(filepath);
}

//--------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{

    std::string cloudPointFile("./20191211-brain.ply");
    if (argc > 1) {
        cloudPointFile = std::string(argv[argc - 1]);
    }

    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Create the pico api
    pico::ApiInit pico_init{ hInstance };
    auto result = pico::api::create(pico_init);

    if (!result) {
        std::clog << "Pico api failed to create ?" << std::endl;
        return 1;
    }

    // First a device, aka the gpu api used by pico
    pico::DeviceInit deviceInit {};
    auto gpuDevice = pico::api::createDevice(deviceInit);


    // Content creation
    pico::vec4 viewportRect{ 0.0f, 0.0f, 1280.0f, 720.f };
    float doAnimate = 1.0f;

    // Some content, why not a pointcloud ?
    auto pointCloud = createPointCloud(cloudPointFile);

    // Let's allocate buffer to hold the point cloud mesh
    pico::BufferInit vertexBufferInit{};
    vertexBufferInit.usage = pico::ResourceUsage::VERTEX_BUFFER;
    vertexBufferInit.hostVisible = true;
    vertexBufferInit.bufferSize = pointCloud->_mesh->_vertexBuffers._buffers[0]->getSize();
    vertexBufferInit.vertexStride = pointCloud->_mesh->_vertexBuffers._streamLayout.evalBufferViewByteStride(0);

    auto vertexBuffer = gpuDevice->createBuffer(vertexBufferInit);
    memcpy(vertexBuffer->_cpuMappedAddress, pointCloud->_mesh->_vertexBuffers._buffers[0]->_data.data(), vertexBufferInit.bufferSize);

    auto numVertices = pointCloud->_mesh->getNumVertices();

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
        pointCloud->_mesh->_vertexBuffers._streamLayout,
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

    // A Camera to look at the scene
    auto camera = std::make_shared<pico::Camera>();
    camera->setAspectRatio((viewportRect.z / viewportRect.w));
    camera->setEye({ 0.5f, 1.0f, 2.04f });
    camera->setOrientation({ 1.f, 0.f, 0.0f },{ 0.f, 1.f, 0.f });

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

        pico::vec4 clearColor(14.f/255.f, 14.f / 255.f, 14.f / 255.f, 1.f);
        batch->clear(swapchain, currentIndex, clearColor);

        batch->beginPass(swapchain, currentIndex);

        batch->setPipeline(pipeline);

        batch->bindVertexBuffers(1, &vertexBuffer);

        batch->setViewport(viewportRect);
        batch->setScissor(viewportRect);

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
    auto window =pico::api::createWindow(windowInit);

    
    pico::SwapchainInit swapchainInit { (uint32_t)viewportRect.z, (uint32_t)viewportRect.w, (HWND) window->nativeWindow(), true };
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

    windowHandler->_onKeyboardDelegate = [&](const pico::KeyboardEvent& e) {
        if (e.state && e.key == pico::KEY_SPACE) {
            doAnimate = (doAnimate == 0.f ? 1.0f : 0.0f);
        }
        if (e.state && e.key == pico::KEY_3) {
            // look down
            camera->setOrientation({ 1.f, 0.f, 0.f }, { 0.f, 0.f, -1.f });
            camera->setEye({ 0.5f, 3.f, 0.f });
        }
        if (e.state && e.key == pico::KEY_1) {
            // look side
            camera->setOrientation({ 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.0f });
            camera->setEye({ 0.5f, 0.6f, 2.f });
        }
        if (e.state && e.key == pico::KEY_2) {
            // look lateral
            camera->setOrientation({ 0.f, 0.f, -1.f }, { 0.f, 1.f, 0.0f });
            camera->setEye({ 2.5f, 0.6f, 0.f });
        }
        if (e.state && e.key == pico::KEY_4) {
            // look 3/4 down
            camera->setOrientation({ 1.f, 0.f, -1.f }, { 0.f, 1.f, -1.0f });
            camera->setEye({ 2.f, 2.f, 2.f });
        }

        if (e.state && e.key == pico::KEY_UP) {
            camera->setEye(camera->getEye() + camera->getFront() * 0.1f);
        }
        if (e.state && e.key == pico::KEY_DOWN) {
            camera->setEye(camera->getEye() + camera->getBack() * 0.1f);
        }
        if (e.state && e.key == pico::KEY_LEFT) {
            camera->setEye(camera->getEye() + camera->getLeft() * 0.1f);
        }
        if (e.state && e.key == pico::KEY_RIGHT) {
            camera->setEye(camera->getEye() + camera->getRight() * 0.1f);
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
