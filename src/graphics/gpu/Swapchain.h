// Swapchain.h 
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
#pragma once

#include <core/math/Math3D.h>
#include <core/api.h>

#include "gpu.h"

namespace graphics {

    struct VISUALIZATION_API SwapchainInit {
#ifdef _WINDOWS
        HWND hWnd;
#endif  

        uint32_t width;
        uint32_t height;

        // No depth buffer by default
        bool     depthBuffer{ false };

        PixelFormat colorBufferFormat{ defaultColorBufferFormat() };
    };

    class VISUALIZATION_API Swapchain {
    protected:
        // Swapchain is created from the device
        friend class Device;
        Swapchain();

    public:
        ~Swapchain();

        SwapchainInit _init;
        uint8_t _currentIndex;

        uint8_t currentIndex() const;

        uint32_t width() const { return _init.width; }
        uint32_t height() const { return _init.height; }
        PixelFormat colorBufferFormat() const { return _init.colorBufferFormat; }

        core::vec4 viewportRect() const { return core::vec4( 0, 0, width(), height());}
    };
}