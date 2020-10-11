// pico_interop-gdi.cpp 
//
// Sam Gateau - FEbruary 2020
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

#include <pico/gpu/Device.h>
#include <pico/gpu/Batch.h>
#include <pico/gpu/Swapchain.h>
#include <pico/render/Renderer.h>

#include <pico/gpu/Pipeline.h>
#include <pico/gpu/Resource.h>
#include <pico/gpu/Descriptor.h>
#include <pico/gpu/Shader.h>
#include <pico/gpu/gpu.h>

#include <uix/Window.h>

#pragma comment(lib, "msimg32.lib")

#include <mutex>

//--------------------------------------------------------------------------------------

struct GDCRenderer {

    GDCRenderer() {
    }
    ~GDCRenderer() {
        resize(0, 0, 0, 0);
    }

    std::mutex _mutex;

    uint32_t _width;
    uint32_t _height;
    HDC hdcOffscreen{ 0 };
    HBITMAP bitmap;
    BITMAPINFOHEADER   bi;
    HANDLE hDIB{ 0 };
    DWORD dwBmpSize = 0;

    std::vector<uint8_t> pixels;

    pico::BufferPointer pixelBuffer;


    void resize(const pico::DevicePointer& device, HDC srcgdc, uint32_t width, uint32_t height) {
        if (pixelBuffer) {
            pixelBuffer.reset();
        }
        if (bitmap) {
            DeleteObject(bitmap);
            bitmap = 0;
        }
        if (hdcOffscreen) {
            DeleteObject(hdcOffscreen);
            hdcOffscreen = 0;
        }
        if (hDIB) {
            GlobalFree(hDIB);
            hDIB = 0;
        }

        // Early exit all clean
        if (width <= 0 || height <= 0) {
            return;
        }


        hdcOffscreen = CreateCompatibleDC(srcgdc);
        bitmap = CreateCompatibleBitmap(srcgdc, width, height);
        HBITMAP old_bitmap = (HBITMAP)SelectObject(hdcOffscreen, bitmap);

        _width = width;
        _height = height;
    
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = width;
        bi.biHeight = height;
        bi.biPlanes = 1;
        bi.biBitCount = 32;
        bi.biCompression = BI_RGB;
        bi.biSizeImage = 0;
        bi.biXPelsPerMeter = 0;
        bi.biYPelsPerMeter = 0;
        bi.biClrUsed = 0;
        bi.biClrImportant = 0;


        dwBmpSize = ((_width * 32 + 31) / 32) * 4 * _height;

        // Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that 
        // call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
        // have greater overhead than HeapAlloc.
        hDIB = GlobalAlloc(GHND, dwBmpSize);

        pixels.resize(dwBmpSize);

        pico::BufferInit bufferInit{};
        bufferInit.bufferSize = dwBmpSize;
        bufferInit.hostVisible = true;
        pixelBuffer = device->createBuffer(bufferInit);
    }
    
    void render(float t) {
        HBITMAP old_bitmap = (HBITMAP)SelectObject(hdcOffscreen, bitmap);

        TRIVERTEX vertices[2];
        ZeroMemory(&vertices, sizeof(vertices));
        vertices[0].Red = uint16_t(0xFF00 * t);
        vertices[0].Green = 0x0000;
        vertices[0].Blue = 0x0000;
        vertices[0].x = 0;
        vertices[0].y = 0;

        vertices[1].Red = 0x0000;
        vertices[1].Green = 0x0000;
        vertices[1].Blue = 0xFF00;
        vertices[1].x = _width;
        vertices[1].y = _height;

        GRADIENT_RECT rects[1];
        ZeroMemory(&rects, sizeof(rects));
        rects[0].UpperLeft = 0;
        rects[0].LowerRight = 1;

        GradientFill(hdcOffscreen, vertices, 2, rects, 1, GRADIENT_FILL_RECT_V);
        //    BitBlt(hdc, 0, 0, c_nWndWidth, c_nWndHeight, hdcOffscreen, 0, 0, SRCCOPY);

        std::string text = "Ca Marche ";
        RECT rect;
        SetTextColor(hdcOffscreen, 0xFFFFFFFF);
        SetBkMode(hdcOffscreen, TRANSPARENT);
        rect.left = 0;
        rect.top = 100;
        rect.bottom = 300;
        rect.right = 500;
        DrawText(hdcOffscreen, text.c_str(), text.length(), &rect, DT_SINGLELINE | DT_NOCLIP);

        // Lock 
        char* lpbitmap = (char*)GlobalLock(hDIB);

        // Gets the "bits" from the bitmap and copies them into a buffer 
        // which is pointed to by lpbitmap.
        GetDIBits(hdcOffscreen, bitmap, 0,
            (UINT)_height,
            lpbitmap,
            (BITMAPINFO*)&bi, DIB_RGB_COLORS);

        GlobalUnlock(hDIB);
        memcpy(pixels.data(), lpbitmap, dwBmpSize);

        
        memcpy(pixelBuffer->_cpuMappedAddress, pixels.data(), dwBmpSize);

    }
};

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

   // Content creation

    // Renderer creation

    // First a device, aka the gpu api used by pico
    pico::DeviceInit deviceInit { "D3D12" };
    auto gpuDevice = pico::Device::createDevice(deviceInit);

 
    // Presentation creation

    // We need a window where to present, let s use the pico::Window for convenience
    // This could be any window, we just need the os handle to create the swapchain next.
    auto windowHandler = new uix::WindowHandlerDelegate();
    uix::WindowInit windowInit { windowHandler };
    auto window = uix::Window::createWindow(windowInit);

    pico::SwapchainInit swapchainInit { 640, 480, (HWND) window->nativeWindow() };
    auto swapchain = gpuDevice->createSwapchain(swapchainInit);

    // Let's try to do something in a DC:
    auto gdcRenderer = std::make_shared<GDCRenderer>();
    gdcRenderer->resize(gpuDevice, GetDC((HWND)window->nativeWindow()), window->width(), window->height());

    pico::TextureInit textureInit { window->width(), window->height() };
    auto textureForGDI = gpuDevice->createTexture(textureInit);

    pico::SamplerInit samplerInit {};
    auto sampler = gpuDevice->createSampler(samplerInit);


    // Let's allocate buffer
    // quad
    std::vector<float> vertexData = {
        -0.25f,  1.0f, 0.0f, 1.0f,
        -0.25f, -0.25f, 0.0f, 1.0f,
        1.0f, -0.25f, 0.0f, 1.0f,
        1.0f,  1.0f, 0.0f, 1.0f,
    };

    vertexData[4 * 0 + 0] += 0.5f;
    vertexData[4 * 1 + 0] += 0.5f;
    vertexData[4 * 2 + 0] += 0.5f;
    vertexData[4 * 3 + 0] += 0.5f;

    pico::BufferInit vertexBufferInit{};
    vertexBufferInit.usage = pico::ResourceUsage::VERTEX_BUFFER;
    vertexBufferInit.hostVisible = true;
    vertexBufferInit.bufferSize = sizeof(float) * vertexData.size();
    vertexBufferInit.vertexStride = sizeof(float) * 4;
    auto vertexBuffer = gpuDevice->createBuffer(vertexBufferInit);
    memcpy(vertexBuffer->_cpuMappedAddress, vertexData.data(), vertexBufferInit.bufferSize);

    std::vector<uint32_t> indexData = {
        0, 2, 1,
        0, 2, 3
    };
    pico::BufferInit indexBufferInit{};
    indexBufferInit.usage = pico::ResourceUsage::INDEX_BUFFER;
    indexBufferInit.hostVisible = true;
    indexBufferInit.bufferSize = sizeof(uint32_t) * indexData.size();
    auto indexBuffer = gpuDevice->createBuffer(indexBufferInit);
    memcpy(indexBuffer->_cpuMappedAddress, indexData.data(), vertexBufferInit.bufferSize);


    // Declare the vertex format
    pico::AttribArray<1> attribs{ {{ pico::AttribSemantic::A, pico::AttribFormat::VEC4, 0 }} };
    pico::AttribBufferViewArray<1> bufferViews;
    auto vertexLayout = pico::StreamLayout::build(attribs, bufferViews);


    // Let's describe the pipeline Descriptors layout
    pico::DescriptorLayouts descriptorLayouts{
        { pico::DescriptorType::RESOURCE_TEXTURE, pico::ShaderStage::PIXEL, 0, 1},
        { pico::DescriptorType::SAMPLER, pico::ShaderStage::PIXEL, 0, 1},
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
        vertexLayout,
        pico::PrimitiveTopology::TRIANGLE,
        descriptorSetLayout
    };
    pico::PipelineStatePointer pipeline = gpuDevice->createPipelineState(pipelineInit);

    // It s time to create a descriptorSet that matches the expected pipeline descriptor set
    // then we will assign a texture and sampler
    pico::DescriptorSetInit descriptorSetInit{
        descriptorSetLayout
    };
    auto descriptorSet = gpuDevice->createDescriptorSet(descriptorSetInit);


    pico::DescriptorObject texDescriptorObject;
    texDescriptorObject._textures.push_back(textureForGDI);
    pico::DescriptorObject samplerDescriptorObject;
    samplerDescriptorObject._samplers.push_back(sampler);
    pico::DescriptorObjects descriptorObjects = {
        texDescriptorObject, samplerDescriptorObject
    };
    gpuDevice->updateDescriptorSet(descriptorSet, descriptorObjects);


    // And now a render callback where we describe the rendering sequence
    pico::RenderCallback renderCallback = [&, gdcRenderer, textureForGDI](const pico::CameraPointer& camera, const pico::SwapchainPointer& swapchain, const pico::DevicePointer& device, const pico::BatchPointer& batch) {
        core::vec4 viewportRect{ 0.0f, 0.0f, 640.0f, 480.f };

        auto currentIndex = swapchain->currentIndex();

        batch->begin(currentIndex);

        batch->resourceBarrierTransition(
            pico::ResourceBarrierFlag::NONE,
            pico::ResourceState::PRESENT,
            pico::ResourceState::RENDER_TARGET,
            swapchain, currentIndex, -1);

        static float time = 0.0f;
        time += 1.0f / 60.0f;
        float intPart;
        time = modf(time, &intPart);
        // pico::vec4 clearColor(colorRGBfromHSV(vec3(time, 0.5f, 1.f)), 1.f);
        core::vec4 clearColor(core::colorRGBfromHSV(core::vec3(0.5f, 0.5f, 1.f)), 1.f);
        batch->clear(swapchain, currentIndex, clearColor);

        batch->beginPass(swapchain, currentIndex);

        batch->resourceBarrierTransition(pico::ResourceBarrierFlag::NONE, pico::ResourceState::IMAGE_SHADER_RESOURCE, pico::ResourceState::COPY_DEST, textureForGDI);
        batch->uploadTexture(textureForGDI, gdcRenderer->pixelBuffer);
        batch->resourceBarrierTransition(pico::ResourceBarrierFlag::NONE, pico::ResourceState::COPY_DEST, pico::ResourceState::IMAGE_SHADER_RESOURCE, textureForGDI);
    

        batch->setPipeline(pipeline);
        batch->bindDescriptorSet(descriptorSet);

        batch->bindIndexBuffer(indexBuffer);
        batch->bindVertexBuffers(1, &vertexBuffer);

        batch->setViewport(viewportRect);
        batch->setScissor(viewportRect);

        batch->drawIndexed(6, 0);

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


    // Next, a renderer built on this device which will use this renderCallback
    auto renderer = std::make_shared<pico::Renderer>(gpuDevice, renderCallback);

    //Now that we have created all the elements, 
    // We configure the windowHandler onPaint delegate of the window to do real rendering!
    windowHandler->_onPaintDelegate = ([swapchain, renderer, gdcRenderer](const uix::PaintEvent& e) {
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

        gdcRenderer->render(frameCounter / 60.0f);

        

        // Render!
        renderer->render(nullptr, swapchain);
    });

    // Render Loop 
    bool keepOnGoing = true;
    while (keepOnGoing) {
        keepOnGoing = window->messagePump();

    }

    core::api::destroy();

     return 0;
}
