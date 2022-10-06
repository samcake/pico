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

#include <core/math/LinearAlgebra.h>
#include "../gpu/gpu.h"

namespace graphics {

    using float2 = core::vec2;
    using float3 = core::vec3;
    using float4 = core::vec4;
    using int4 = core::ivec4;

    // Render types
    class Scene;
    using ScenePointer = std::shared_ptr<Scene>;

    class Camera;
    using CameraPointer = std::shared_ptr<Camera>;

    struct RenderArgs;
    class Renderer;
    using RendererPointer = std::shared_ptr<Renderer>;

    class Viewport;
    using ViewportPointer = std::shared_ptr<Viewport>;
    struct ViewportInit;

    class Mesh;
    using MeshPointer = std::shared_ptr<Mesh>;

    class Sky;
    using SkyPointer = std::shared_ptr<Sky>;
    using Sky_sp = SkyPointer;

    class SkyDrawableFactory;
    using SkyDrawableFactory_sp = std::shared_ptr<SkyDrawableFactory>;

}

