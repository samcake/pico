// pico_two.cpp 
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

#include <chrono>

#include <core/api.h>

#include <graphics/gpu/Device.h>
#include <graphics/gpu/Resource.h>
#include <graphics/gpu/Shader.h>
#include <graphics/gpu/Pipeline.h>
#include <graphics/gpu/Batch.h>
#include <graphics/gpu/Swapchain.h>
#include <graphics/gpu/gpu.h>
#include <graphics/render/Renderer.h>

#include <uix/Window.h>
#include <uix/Imgui.h>

#include <vector>


//--------------------------------------------------------------------------------------
// pico 2: Draw a triangle and use ImGui
// introducing:
// gpu::Batch
// gpu::Shader
// gpu::Pipeline State 
// gpu::Buffer as Vertex & Index buffer
// gpu::StreamLayout
// render::RenderCallback
//
// uix::Imgui

//--------------------------------------------------------------------------------------

const std::string vertexShaderSource = std::string(R"HLSL(
/*struct ModelViewProjection
{
    matrix MVP;
};
*/
//ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

struct VertexPosColor
{
    float3 Position : POSITION;
};

struct VertexShaderOutput
{
    float4 Color    : COLOR;
    float4 Position : SV_Position;
};

VertexShaderOutput mainVertex(VertexPosColor IN)
{
    VertexShaderOutput OUT;

    // OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 1.0f));
    OUT.Position = float4(IN.Position, 1.0f);
    OUT.Color = float4(1.0f, 0.0f, 0.0f, 1.0f);

    return OUT;
}
)HLSL");

const std::string pixelShaderSource = std::string(R"HLSL(
struct PixelShaderInput
{
    float4 Color : COLOR;
};

float4 mainPixel(PixelShaderInput IN) : SV_Target
{
    return IN.Color;
}
)HLSL");


const std::string& getVertexShaderSource() { return vertexShaderSource; }
const std::string& getPixelShaderSource() { return pixelShaderSource; }

graphics::PipelineStatePointer createPipelineState(const graphics::DevicePointer& device, graphics::StreamLayout streamLayout ) {

    graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "mainVertex", getVertexShaderSource };
    graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);


    graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "mainPixel", getPixelShaderSource };
    graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

    graphics::ProgramInit programInit { vertexShader, pixelShader };
    graphics::ShaderPointer programShader = device->createProgram(programInit);



    graphics::GraphicsPipelineStateInit pipelineInit { programShader, nullptr, streamLayout, graphics::PrimitiveTopology::TRIANGLE };
    graphics::PipelineStatePointer pipeline = device->createGraphicsPipelineState(pipelineInit);

    return pipeline;
}

//--------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Create the pico api
    core::ApiInit pico_init{ };
    auto result = core::api::create(pico_init);

    if (!result) {
        picoLog("Pico api failed to create ?");
        return 1;
    }


    // Renderer creation

    // First a device, aka the gpu api used by pico
  //  graphics::DeviceInit deviceInit{};
    graphics::DeviceInit deviceInit { "VK" };
    auto gpuDevice = graphics::Device::createDevice(deviceInit);


    // Content creation

    // Let's allocate buffer
    // quad
    std::vector<float> vertexData = {
        -0.25f,  0.25f, 0.0f, 1.0f,
        -0.25f, -0.25f, 0.0f, 1.0f,
        0.25f, -0.25f, 0.0f, 1.0f,
        0.25f,  0.25f, 0.0f, 1.0f,
    };

    vertexData[4 * 0 + 0] += 0.5f;
    vertexData[4 * 1 + 0] += 0.5f;
    vertexData[4 * 2 + 0] += 0.5f;
    vertexData[4 * 3 + 0] += 0.5f;

    graphics::BufferInit vertexBufferInit{};
    vertexBufferInit.usage = graphics::ResourceUsage::VERTEX_BUFFER;
    vertexBufferInit.hostVisible = true;
    vertexBufferInit.bufferSize = sizeof(float) * vertexData.size();
    vertexBufferInit.vertexStride = sizeof(float) * 4;
    auto vertexBuffer = gpuDevice->createBuffer(vertexBufferInit);
    memcpy(vertexBuffer->_cpuMappedAddress, vertexData.data(), vertexBufferInit.bufferSize);

    std::vector<uint32_t> indexData = {
        0, 2, 1,
        0, 2, 3
    };
    graphics::BufferInit indexBufferInit{};
    indexBufferInit.usage = graphics::ResourceUsage::INDEX_BUFFER;
    indexBufferInit.hostVisible = true;
    indexBufferInit.bufferSize = sizeof(uint32_t) * indexData.size();
    auto indexBuffer = gpuDevice->createBuffer(indexBufferInit);
    memcpy(indexBuffer->_cpuMappedAddress, indexData.data(), vertexBufferInit.bufferSize);


    // Declare the vertex format
    graphics::AttribArray<1> attribs {{{ graphics::AttribSemantic::A, graphics::AttribFormat::VEC4, 0 }}};
    graphics::AttribBufferViewArray<1> bufferViews;
    auto vertexLayout = graphics::StreamLayout::build(attribs, bufferViews);

    // And a Pipeline
    graphics::PipelineStatePointer pipeline = createPipelineState(gpuDevice, vertexLayout);

    // And now a render callback where we describe the rendering sequence
    graphics::RenderCallback renderCallback = [&](graphics::RenderArgs& args) {
        core::vec4 viewportRect { 0.0f, 0.0f, 640.0f, 480.f };

        auto currentIndex = args.swapchain->currentIndex();

        args.batch->begin(currentIndex);

        args.batch->resourceBarrierTransition(
            graphics::ResourceBarrierFlag::NONE,
            graphics::ResourceState::PRESENT,
            graphics::ResourceState::RENDER_TARGET,
            args.swapchain, currentIndex, -1);

        static float time = 0.0f;
        time += 1.0f / 60.0f;
        float intPart;
        time = modf(time, &intPart);
       // graphics::vec4 clearColor(colorRGBfromHSV(vec3(time, 0.5f, 1.f)), 1.f);
        core::vec4 clearColor(core::colorRGBfromHSV(core::vec3(0.5f, 0.5f, 1.f)), 1.f);
        args.batch->clear(args.swapchain, currentIndex, clearColor);

        args.batch->beginPass(args.swapchain, currentIndex);

        args.batch->bindPipeline(pipeline);

        args.batch->bindIndexBuffer(indexBuffer);
        args.batch->bindVertexBuffers(1, &vertexBuffer);

        args.batch->setViewport(viewportRect);
        args.batch->setScissor(viewportRect);

        args.batch->drawIndexed(6, 0);

        uix::Imgui::draw(args.batch);

        args.batch->endPass();

        args.batch->resourceBarrierTransition(
            graphics::ResourceBarrierFlag::NONE,
            graphics::ResourceState::RENDER_TARGET,
            graphics::ResourceState::PRESENT,
            args.swapchain, currentIndex, -1);

        args.batch->end();

        args.device->executeBatch(args.batch);

        args.device->presentSwapchain(args.swapchain);
    };


    // Next, a renderer built on this device which will use this renderCallback
    auto renderer = std::make_shared<graphics::Renderer>(gpuDevice, renderCallback);


    // Presentation creation

    // We need a window where to present, let s use the graphics::Window for convenience
    // This could be any window, we just need the os handle to create the swapchain next.
    auto windowHandler = new uix::WindowHandlerDelegate();
    uix::WindowInit windowInit { windowHandler };
    auto window = uix::Window::createWindow(windowInit);


    // Setup Dear ImGui context with the gpuDevice and the brand new window
    uix::Imgui::create();
    uix::Imgui::setup(window, gpuDevice);

    graphics::SwapchainInit swapchainInit { (HWND)window->nativeWindow(), 640, 480 };
    auto swapchain = gpuDevice->createSwapchain(swapchainInit);

    //Now that we have created all the elements, 
    // We configure the windowHandler onPaint delegate of the window to do real rendering!
    windowHandler->_onPaintDelegate = ([swapchain, renderer](const uix::PaintEvent& e) {
        // Measuring framerate
        static uint64_t numSixtyFrame = 0;
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
            numSixtyFrame++;
        }


        uix::Imgui::newFrame();

        ImGui::Begin("Hello, world!");
        ImGui::Text("This is some useful text.");
        ImGui::Text((std::to_string(numSixtyFrame) + " - " + std::to_string(frameCounter)).c_str());
        ImGui::End();
        ImGui::Render();

        // Render!
        renderer->render(nullptr, swapchain);
    });

    // On resize deal with it
    windowHandler->_onResizeDelegate = [&](const uix::ResizeEvent& e) {
        // only resize the swapchain when we re done with the resize
        //if (e.over)
        gpuDevice->flush();
        gpuDevice->resizeSwapchain(swapchain, e.width, e.height);
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
