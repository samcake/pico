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
#include "Camera.h"

namespace graphics {

    struct RenderArgs {
        DevicePointer device;
        BatchPointer batch;
        SwapchainPointer swapchain;
        CameraPointer camera;
        BatchTimerPointer timer;
        DescriptorSetPointer viewPassDescriptorSet;
        ScenePointer scene;
    };

    using RenderCallback = std::function<void(RenderArgs& renderArgs)>;

    struct AnimateArgs {
        float time = 0;
        DevicePointer device;
        ScenePointer scene;
    };

    using AnimateCallback = std::function<void(AnimateArgs& animateArgs)>;


    class VISUALIZATION_API Renderer {
    public:
        Renderer(const DevicePointer& device, RenderCallback renderCallback, AnimateCallback animateCallback);
        ~Renderer();

        void render(const CameraPointer& camera, const SwapchainPointer& swapchain, RenderCallback callback = nullptr);
        void animate(float time, AnimateCallback animCallback = nullptr);

    protected:
        DevicePointer _device;
        BatchPointer _batch;
        RenderCallback _renderCallback;
        AnimateCallback _animateCallback;
    };
}
