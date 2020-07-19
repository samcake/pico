// Renderer.h 
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

#include "render.h"
#include <functional>

namespace pico {

    using RenderCallback = std::function<void(const CameraPointer & camera, const SwapchainPointer & swapchain, const DevicePointer & device, const BatchPointer & batch)>;

    class VISUALIZATION_API Renderer {
    public:
        Renderer(const DevicePointer& device, RenderCallback callback);
        ~Renderer();

        void render(const CameraPointer& camera, const SwapchainPointer& swapchain);

    protected:
#pragma warning(push)
#pragma warning(disable: 4251)
        DevicePointer _device;
        BatchPointer _batch;
        RenderCallback _callback;

#pragma warning(pop)
    };
}
