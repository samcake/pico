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
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;

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
Buffer<float4> rectBuffer : register(t0);

struct VertexShaderOutput
{
    float4 coords : TEXCOORD;
    float4 position : SV_Position;
};

VertexShaderOutput mainVertex(uint ivid : SV_VertexID)
{
    VertexShaderOutput OUT;

    const int num_tris = 1;
    uint vid = ivid % (3 * num_tris);
    uint instance = ivid / (3 * num_tris);
    uint tvid = vid % 3;
    uint tid = vid / 3;
    
    
    float3 position = float3(0.0, 0.0, 0.0);
    float3 color = float3(1.0, 1.0, 1.0);

    position.xy = float2(((tvid == 1) ? 2.0 : 0.0), ((tvid == 2) ? 2.0 : 0.0));

    float4 rect = rectBuffer[instance];
    
    float4 coords = float4(position.xy * rect.zw + rect.xy, 0, 1.0);
    OUT.position = coords;
    OUT.coords = float4(position.xy, 0.0f, 1.0f);

    return OUT;
}

)HLSL");

const std::string pixelShaderSource = std::string(R"HLSL(

struct PixelShaderInput
{
    float4 coords : TEXCOORD;
};

float4 mainPixel(PixelShaderInput IN) : SV_Target
{
    float maxCoord = max(IN.coords.x, IN.coords.y);
    if (maxCoord > 1.0) discard;

    float3 color = lerp(
                       lerp(float3(1, 0, 0), float3(0, 1, 0), IN.coords.x),
                       lerp(float3(0, 0, 1), float3(1, 1, 0), IN.coords.x),
                       IN.coords.y);


    return float4(color, 1);
}
)HLSL");


const std::string& getVertexShaderSource() { return vertexShaderSource; }
const std::string& getPixelShaderSource() { return pixelShaderSource; }

graphics::PipelineStatePointer createPipelineState(const graphics::DevicePointer& device, const graphics::RootDescriptorLayoutPointer& rootDescriptorLayout) {

    graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "mainVertex", getVertexShaderSource };
    graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);


    graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "mainPixel", getPixelShaderSource };
    graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

    graphics::ProgramInit programInit { vertexShader, pixelShader };
    graphics::ShaderPointer programShader = device->createProgram(programInit);



    graphics::GraphicsPipelineStateInit pipelineInit{
        programShader,
        rootDescriptorLayout,
        graphics::StreamLayout(),
        graphics::PrimitiveTopology::TRIANGLE
    };
    graphics::PipelineStatePointer pipeline = device->createGraphicsPipelineState(pipelineInit);

    return pipeline;
}

struct Node
{
    std::string path;
    int64_t size;
    int32_t parent = -1;
};
std::vector<Node> createNodeTree(const std::string& rootPath)
{
    std::vector<Node> tree = { };
    std::vector<std::string> queue = { rootPath };
    std::vector<int32_t> traversalParentID = { };
    std::vector<int32_t> traversalNumSubFolders = { };
    std::vector<int64_t> traversalSizes = { };

    while (!queue.empty())
    {
        auto currentDirPath = queue.back();
        queue.pop_back();

        const std::string dir_tab("| ");
        const std::string dir_prefix("+ ");
        std::string dir_padding;
        for (auto i:traversalParentID) dir_padding += dir_tab;
        std::string padding = dir_padding + dir_tab;
        dir_padding += dir_prefix;

        {
            int32_t currentDirParentID = -1;
            if (!traversalParentID.empty()) {
                currentDirParentID = traversalParentID.back();
            }

            Node currentDirNode = { currentDirPath, 0, currentDirParentID };
            tree.emplace_back(currentDirNode);
            picoOut("{}{}", dir_padding, currentDirPath);
        }
        int32_t currentDirID = tree.size() - 1;
        int64_t currentDirFilesSize = 0;

        std::vector<std::string> subfolders;
        for (const auto& entry : fs::directory_iterator(currentDirPath)) {
            if (entry.is_directory())
            {
                subfolders.emplace_back(entry.path().generic_string());
            }
            else if (entry.is_regular_file())
            {
                picoOut("{}{}", padding, entry.path().generic_string());
                Node n = { entry.path().generic_string(), entry.file_size(), currentDirID };
                tree.emplace_back(n);
                currentDirFilesSize += n.size;
            }
            else
            {
                picoOut("WTF {}", entry.path().generic_string());
            }
        }

        tree[currentDirID].size = currentDirFilesSize;
        
        // Accumulated filles size to the parent of the current dir
        if (!traversalSizes.empty())
            traversalSizes.back() += currentDirFilesSize;

        if (subfolders.size())
        {
            // Add the currentDir to the traversal path and add it to the queue dive in
            traversalParentID.emplace_back(currentDirID);
            traversalNumSubFolders.emplace_back(subfolders.size());
            traversalSizes.emplace_back(0);

            for (auto it = subfolders.rbegin(); it != subfolders.rend(); ++it)
            {
                queue.emplace_back(*it);
            }
        }
        else
        {
            // Just went through a sub folder of the traversalfolder tail
            // remove one from the count,
            // if reach 0 then we know we are going back up one level
            traversalNumSubFolders.back()--;
            if (traversalNumSubFolders.back() <= 0)
            {
                auto currentTraversedSize = traversalSizes.back();
                tree[traversalParentID.back()].size += currentTraversedSize;

                traversalSizes.pop_back();
                traversalParentID.pop_back();
                traversalNumSubFolders.pop_back();

                tree[traversalParentID.back()].size += currentTraversedSize;

            }
        }
    }

    return tree;
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

   // auto tree = createNodeTree("C:/sam/dev/pico/test");
    auto tree = createNodeTree("C:/sam/temp");

    // Renderer creation

    // First a device, aka the gpu api used by pico
    graphics::DeviceInit deviceInit {};
    auto gpuDevice = graphics::Device::createDevice(deviceInit);


    // Content creation

    // Let's allocate buffer
    // quad
    std::vector<core::vec4> rectData = {
        {-0.2, -0.2, 1.f, 1.f}
    };

    graphics::BufferInit rectBufferInit{};
    rectBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
    rectBufferInit.hostVisible = true;
    rectBufferInit.bufferSize = sizeof(core::vec4) * rectData.size();
    rectBufferInit.numElements = rectData.size();
    rectBufferInit.structStride = sizeof(core::vec4);

    auto rectBuffer = gpuDevice->createBuffer(rectBufferInit);
    memcpy(rectBuffer->_cpuMappedAddress, rectData.data(), rectBufferInit.bufferSize);
 
    // Let's describe the pipeline Descriptors layout
    graphics::RootDescriptorLayoutInit rootLayoutInit{
        {
        },
        {
           {
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::ALL_GRAPHICS, 0, 1} // Rect Buffer
           }
        }
    };
    auto rootDescriptorLayout = gpuDevice->createRootDescriptorLayout(rootLayoutInit);

    // And a Pipeline
    graphics::PipelineStatePointer pipeline = createPipelineState(gpuDevice, rootDescriptorLayout);

    graphics::DescriptorSetInit descriptorSetInit{
        pipeline->getRootDescriptorLayout(),
        0,
        false
    };
    graphics::DescriptorSetPointer descriptorSet = gpuDevice->createDescriptorSet(descriptorSetInit);
    graphics::DescriptorObjects descriptorObjects = {
            { graphics::DescriptorType::RESOURCE_BUFFER, rectBuffer},
    };
    gpuDevice->updateDescriptorSet(descriptorSet, descriptorObjects);

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
       // core::vec4 clearColor(colorRGBfromHSV(vec3(time, 0.5f, 1.f)), 1.f);
       // core::vec4 clearColor(core::colorRGBfromHSV(core::vec3(0.5f, 0.5f, 1.f)), 1.f);
        core::vec4 clearColor(0.3, 0.3, 0.34, 1.f);
        args.batch->clear(args.swapchain, currentIndex, clearColor);

        args.batch->beginPass(args.swapchain, currentIndex);

        args.batch->bindPipeline(pipeline);

        args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet);

        args.batch->setViewport(viewportRect);
        args.batch->setScissor(viewportRect);

        args.batch->draw(3 * rectBuffer->numElements(), 0);

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
    auto renderer = std::make_shared<graphics::Renderer>(gpuDevice, renderCallback, nullptr);


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
