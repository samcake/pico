// render.h 
//
// Sam Gateau - June 2020
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

#include "gpu/gpu.h"

namespace pico {

    // Render types
    class Scene;
    using ScenePointer = std::shared_ptr<Scene>;

    class Camera;
    using CameraPointer = std::shared_ptr<Camera>;

    class Renderer;
    using RendererPointer = std::shared_ptr<Renderer>;

    class Viewport;
    using ViewportPointer = std::shared_ptr<Viewport>;

    class Mesh;
    using MeshPointer = std::shared_ptr<Mesh>;

    class PointCloud;
    using PointCloudPointer = std::shared_ptr<PointCloud>;

    class DrawcallObject;
    using DrawcallObjectPointer = std::shared_ptr<DrawcallObject>;


}

