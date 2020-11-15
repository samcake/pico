// Drawable.h 
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

#include <functional>

#include <core/math/LinearAlgebra.h>

#include "Renderer.h"
#include "Scene.h"

namespace graphics {

    using DrawObjectCallback = std::function<void(
        const NodeID node,
        const CameraPointer& camera,
        const SwapchainPointer& swapchain,
        const DevicePointer& device,
        const BatchPointer& batch)>;

    // Here we define the DrawcallObject as the container of the various pico gpu objects we need to render an item.
    // this will evolve and probably clean up over time and move the genralized concepts in the visualization library
    class VISUALIZATION_API DrawcallObject {
    public:
#pragma warning(push)
#pragma warning(disable: 4251)
        DrawObjectCallback _drawCallback;
        core::Bounds _bounds;

#pragma warning(pop)

        DrawcallObject() : _drawCallback(nullptr) {}
        DrawcallObject(DrawObjectCallback callback) : _drawCallback(callback) {}

        void draw(const NodeID node, const CameraPointer& camera,
            const SwapchainPointer& swapchain,
            const DevicePointer& device,
            const BatchPointer& batch);
    };
    using DrawcallObjectPointer = std::shared_ptr<DrawcallObject>;

}

