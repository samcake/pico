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

#include <pico/pico.h>
#include <pico/gpu/Device.h>
#include <pico/gpu/Batch.h>
#include <pico/gpu/Swapchain.h>
#include <pico/window/Window.h>
#include <pico/render/Renderer.h>

#include <pico/gpu/Resource.h>
#pragma comment(lib, "msimg32.lib")

#include <mutex>

//--------------------------------------------------------------------------------------

struct GDCRenderer {

    GDCRenderer() {
    }
    ~GDCRenderer() {
        resize(0, 0, 0);
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

    void resize(HDC srcgdc, uint32_t width, uint32_t height) {
      
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
    }
    
    void render() {
        HBITMAP old_bitmap = (HBITMAP)SelectObject(hdcOffscreen, bitmap);

        TRIVERTEX vertices[2];
        ZeroMemory(&vertices, sizeof(vertices));
        vertices[0].Red = 0xFF00;
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



        // Lock 
        char* lpbitmap = (char*)GlobalLock(hDIB);

        // Gets the "bits" from the bitmap and copies them into a buffer 
        // which is pointed to by lpbitmap.
        GetDIBits(hdcOffscreen, bitmap, 0,
            (UINT)_height,
            lpbitmap,
            (BITMAPINFO*)&bi, DIB_RGB_COLORS);

        GlobalUnlock(hDIB);
        memcpy( pixels.data(), lpbitmap, dwBmpSize);
    }
};

//--------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Create the pico api
    pico::ApiInit pico_init{ };
    auto result = pico::api::create(pico_init);

    if (!result) {
        std::clog << "Pico api failed to create ?" << std::endl;
        return 1;
    }

   // Content creation

    // Renderer creation

    // First a device, aka the gpu api used by pico
    pico::DeviceInit deviceInit { "D3D12-GDI" };
    auto gpuDevice = pico::Device::createDevice(deviceInit);

    // Next, a renderer built on this device
    auto renderer = std::make_shared<pico::Renderer>(gpuDevice, nullptr);

    // Presentation creation

    // We need a window where to present, let s use the pico::Window for convenience
    // This could be any window, we just need the os handle to create the swapchain next.
    auto windowHandler = new pico::WindowHandlerDelegate();
    pico::WindowInit windowInit { windowHandler };
    auto window = pico::Window::createWindow(windowInit);

    pico::SwapchainInit swapchainInit { 640, 480, (HWND) window->nativeWindow() };
    auto swapchain = gpuDevice->createSwapchain(swapchainInit);

    // Let's try to do something in a DC:
    auto gdcRenderer = std::make_shared<GDCRenderer>();
    gdcRenderer->resize(GetDC((HWND)window->nativeWindow()), window->width(), window->height());

    pico::TextureInit textureInit { window->width(), window->height() };
    auto textureForGDI = gpuDevice->createTexture(textureInit);

    //Now that we have created all the elements, 
    // We configure the windowHandler onPaint delegate of the window to do real rendering!
    windowHandler->_onPaintDelegate = ([swapchain, renderer, gdcRenderer](const pico::PaintEvent& e) {
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

        gdcRenderer->render();

        

        // Render!
        renderer->render(nullptr, swapchain);
    });

    // Render Loop 
    bool keepOnGoing = true;
    while (keepOnGoing) {
        keepOnGoing = window->messagePump();

    }

    pico::api::destroy();
    std::clog << "Pico api destroyed" << std::endl;

     return 0;
}
