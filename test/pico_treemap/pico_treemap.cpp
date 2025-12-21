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

#include "Treemap_vert.h"
#include "Treemap_frag.h"

#include <vector>
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;

//--------------------------------------------------------------------------------------
// pico treemap: explore our own treemap
//--------------------------------------------------------------------------------------

graphics::PipelineStatePointer createPipelineState(const graphics::DevicePointer& device) {

    // Let's describe the pipeline Descriptors layout
    graphics::RootDescriptorLayoutInit rootLayoutInit{
        Treemap_vert::getPushLayout()
        ,
        {
            Treemap_vert::getResourceLayout()
        }
    };
    auto rootDescriptorLayout = device->createRootDescriptorLayout(rootLayoutInit);

    graphics::ShaderPointer vertexShader = device->createShader(Treemap_vert::getShaderInit("mainVertex"));
    graphics::ShaderPointer pixelShader = device->createShader(Treemap_frag::getShaderInit("mainPixel"));

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

using IDs = std::vector<int32_t>;

struct Node
{
    std::string path;
    int64_t size;
    int32_t parent = -1;
    int32_t sybling = -1;
    int32_t childBegin = -1;
    int32_t childEnd = -1;
};
using Nodes = std::vector<Node>;

int32_t addNode(Nodes& tree, int32_t parent)
{
    int32_t newChild = tree.size();

    if (parent >= 0)
    {
        int32_t prevChild = tree[parent].childEnd;
        if (prevChild < 0)
        {
            tree[parent].childBegin = tree[parent].childEnd = newChild;
        }
        else
        {
            tree[prevChild].sybling = tree[parent].childEnd = newChild;
        }
    }

    Node n;
    n.parent = parent;
    tree.emplace_back(n);

    return newChild;
}

int32_t nextNode(Nodes& tree, int32_t current, int32_t& depth, bool visitChildren = true)
{
    if (current < 0)
        return -1;

    auto& n = tree[current];
    if (visitChildren && n.childBegin >= 0)
    {
        depth++;
        return n.childBegin;
    }
    else if (n.sybling >= 0)
    {
        return n.sybling;
    }

    // go up
    depth--;
    return nextNode(tree, n.parent, depth, false);
}

Nodes createNodeTree(const std::string& rootPath)
{
    Nodes tree = { };
    std::vector<std::string> queue = { rootPath };
    IDs traversalParentID = { };
    IDs traversalNumSubFolders = { };

    while (!queue.empty())
    {
        auto currentDirPath = queue.back();
        queue.pop_back();

        int32_t currentDirID = -1;
        {
            int32_t currentDirParentID = -1;
            if (!traversalParentID.empty()) {
                currentDirParentID = traversalParentID.back();
            }

            currentDirID = addNode(tree, currentDirParentID);
            tree[currentDirID].path = currentDirPath;
        }
        int64_t currentDirFilesSize = 0;

        std::vector<std::string> subfolders;
        for (const auto& entry : fs::directory_iterator(currentDirPath)) {
            if (entry.is_directory())
            {
                subfolders.emplace_back(entry.path().generic_string());
            }
            else if (entry.is_regular_file())
            {
                int32_t newNodeID = addNode(tree, currentDirID);
                tree[newNodeID].path = entry.path().generic_string();
                tree[newNodeID].size = entry.file_size();

                currentDirFilesSize += entry.file_size();
            }
            else
            {
                picoOut("WTF {}", entry.path().generic_string());
            }
        }

        // accumulated size in current dir from local files is propagated up
        tree[currentDirID].size = currentDirFilesSize;
        for (auto it = traversalParentID.rbegin(); it != traversalParentID.rend(); ++it)
        {
            tree[*it].size += currentDirFilesSize;
        }

        if (subfolders.size())
        {
            // Add the currentDir to the traversal path and add it to the queue dive in
            traversalParentID.emplace_back(currentDirID);
            traversalNumSubFolders.emplace_back(subfolders.size());

            for (auto it = subfolders.rbegin(); it != subfolders.rend(); ++it)
            {
                queue.emplace_back(*it);
            }
        }
        else if (traversalNumSubFolders.size())
        {
            // Just went through a sub folder of the traversalfolder tail
            // remove one from the count,
            // if reach 0 then we know we are going back up one level
            traversalNumSubFolders.back()--;
            if (traversalNumSubFolders.back() <= 0)
            {
                traversalParentID.pop_back();
                traversalNumSubFolders.pop_back();
            }
        }
    }

    return tree;
}

void printNodeTree(Nodes& tree)
{

    int NumNodes = tree.size();
    int NumPrinted = 0;
    IDs traversalFolderID = { };
    IDs traversalNumSubFolders = { };

    const std::string dir_tab("| ");
    const std::string dir_prefix("+ ");

    int next = 0;
    int depth = 0;
    while (NumPrinted < NumNodes)
    {
        auto& n = tree[next];

        std::string dir_padding;
        for (int i = 0; i < depth; ++i) dir_padding += dir_tab;
        std::string padding = dir_padding + dir_tab;
        dir_padding += dir_prefix;

       
        picoOut("{}{} - {}", (n.childBegin < 0 ? padding : dir_padding), n.size, n.path);
    
        next = nextNode(tree, next, depth);
        NumPrinted++;
    }
}

IDs gatherChildren(const Nodes& tree, int32_t node)
{
    IDs children;
    auto& p = tree[node];
    int32_t next = p.childBegin;
    while (next >= 0)
    {
        children.emplace_back(next);

        if (next == p.childEnd)
            break;

        next = tree[next].sybling;
    }

    return children;
}

using RectAreas = std::vector<double>;

double layoutWorst(const RectAreas& collection, IDs& row, double width, double totalArea)
{
    double width2 = width * width;
    double totalArea2 = totalArea * totalArea;

    double maxArea;
    double minArea;
    maxArea = minArea = collection[row[0]];

    for (int i = 1; i < row.size(); ++i)
    {
        maxArea = std::max(maxArea, collection[row[i]]);
        minArea = std::min(minArea, collection[row[i]]);
    }

    return std::max(width2 * maxArea / totalArea2, totalArea2 / (width2 * minArea));
}

void layoutRow(std::vector<core::vec4>& treemap, const IDs& collectionIDs, const RectAreas& collectionAreas, const IDs& collectionOrder, const core::vec4& rootRect, int32_t& next, core::vec2& cornerOffset)
{
    IDs row = { collectionOrder[next] };
    double rowArea = collectionAreas[collectionOrder[next]];
    next++;

    // determine next row direction based on the remaining space shape
    core::vec2 rootSize(rootRect.z - cornerOffset.x, rootRect.w - cornerOffset.y);
    int32_t direction = (rootSize.x > rootSize.y ? 1 : 0); // vertical or horizontal growth
    double rootSide = rootSize[direction];

    // single rect to layout in a row, just skip row placement and occupy the row
    if (next < collectionIDs.size())
    {
        double bestaspectratio = layoutWorst(collectionAreas, row, rootSide, rowArea);

        double newaspectratio = bestaspectratio;
        while (next < collectionIDs.size())
        {
            row.emplace_back(collectionOrder[next]);
            newaspectratio = layoutWorst(collectionAreas, row, rootSide, rowArea);
            if (newaspectratio > bestaspectratio)
            {
                row.pop_back();
                break;
            }

            // keep the rect in the row, add its area to the row
            bestaspectratio = newaspectratio;
            rowArea += collectionAreas[collectionOrder[next]];

            next++; // rect at next is in the row and will be kept, move on
        }
    }

    // Place the rect in the row
    int32_t otherDirection = !direction;
    double rowOtherSide = rowArea / rootSide;
    double sideOffset = cornerOffset[direction];
    double otherSideOffset = cornerOffset[otherDirection];
    for (auto i : row)
    {
        int32_t id = collectionIDs[i];
        double area = collectionAreas[i];
        double rectSide = rootSide * area / rowArea ;
 
        auto & rect = treemap[id];

        rect[direction] = rootRect[direction] + sideOffset;
        rect[otherDirection] = rootRect[otherDirection] + otherSideOffset;
        rect[2 + direction] = rectSide;
        rect[2 + otherDirection] = rowOtherSide;

        sideOffset += rectSide;
    }

    cornerOffset[otherDirection] += rowOtherSide;
}

IDs layoutRect(const Nodes& tree, std::vector<core::vec4>& treemap, int32_t root)
{
    // collect the children of the root
    auto collectionIDs = gatherChildren(tree, root);
    if (collectionIDs.empty())
        return collectionIDs;

    // allocate the areas of the children rect
    RectAreas collectionAreas;
    collectionAreas.reserve(collectionIDs.size());
    int64_t rootSize = tree[root].size;
    const auto& rootRect = treemap[root];
    double rootRectArea = rootRect.z * rootRect.w;
    double rootSizeInv = rootRectArea / (double)rootSize;
    for (auto id : collectionIDs)
    {
        collectionAreas.emplace_back((double) tree[id].size * rootSizeInv);
    }

    // sort the collection IDs and Areas in descending order
    std::vector<int32_t> indices(collectionIDs.size());
    for (int32_t i = 0; i < indices.size(); ++i) {
        indices[i] = i;
    }
    std::sort(indices.begin(), indices.end(), [&collectionAreas](int32_t a, int32_t b) {
        return collectionAreas[a] > collectionAreas[b]; // Use < for ascending, > for descending
    });


    // And layout
    int32_t next = 0;
    core::vec2 offset = 0;
    while (next < collectionIDs.size())
    {
        layoutRow(treemap, collectionIDs, collectionAreas, indices, rootRect, next, offset);
    }

    return collectionIDs;
}

std::vector<core::vec4> buildTreemap(const Nodes& tree, const core::vec2& origin, const core::vec2& size)
{
    std::vector<core::vec4> treemap;
    treemap.resize(tree.size());
    treemap[0] = { origin.x, origin.y, size.x, size.y };

    IDs queue = { 0 };

    while (queue.size())
    {
        auto collection = layoutRect(tree, treemap, queue.back());

        queue.pop_back();
        queue.insert(queue.begin(), collection.begin(), collection.end());
    }


    return treemap;
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
   //auto tree = createNodeTree("C:/Users/samca/Pictures");
   // auto tree = createNodeTree("C:/Users/samca/Pictures/Screenshots");
    auto tree = createNodeTree("C:/Dev/pico");


    printNodeTree(tree);

    // Renderer creation

    // First a device, aka the gpu api used by pico
    graphics::DeviceInit deviceInit {};
    auto gpuDevice = graphics::Device::createDevice(deviceInit);


    // Content creation

    // Let's allocate buffer
    // quad
    std::vector<core::vec4> rectData = buildTreemap(tree, { 0, 0 }, { 640.0, 480.0 });

    graphics::BufferInit rectBufferInit{};
    rectBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
    rectBufferInit.hostVisible = true;
    rectBufferInit.bufferSize = sizeof(core::vec4) * rectData.size();
    rectBufferInit.numElements = rectData.size();
    rectBufferInit.structStride = sizeof(core::vec4);

    auto rectBuffer = gpuDevice->createBuffer(rectBufferInit);
    memcpy(rectBuffer->_cpuMappedAddress, rectData.data(), rectBufferInit.bufferSize);

    // And a Pipeline
    graphics::PipelineStatePointer pipeline = createPipelineState(gpuDevice);

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

        auto currentIndex = args.swapchain->currentIndex();
        core::vec4 viewportRect = args.swapchain->viewportRect();

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

        Treemap_vert::PushUniform odata{ viewportRect.z, viewportRect.w, 1.0f, 1.0f };
        args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, odata);

        args.batch->setViewport(viewportRect);
        args.batch->setScissor(viewportRect);

        args.batch->draw(3 * rectBuffer->numElements(), 0);

       // uix::Imgui::draw(args.batch);

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

        if (e.done)
        {
            std::vector<core::vec4> rectLayout = buildTreemap(tree, {0, 0}, { (float)e.width, (float)e.height });
            memcpy(rectBuffer->_cpuMappedAddress, rectLayout.data(), rectBufferInit.bufferSize);
        }
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
