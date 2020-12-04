// Primitive.h 
//
// Sam Gateau - December 2020
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

#include <memory>
#include <core/math/LinearAlgebra.h>
#include "dllmain.h"

#include <render/Scene.h>
#include <render/Drawable.h>

namespace graphics {
    class Device;
    using DevicePointer = std::shared_ptr<Device>;
    class Camera;
    using CameraPointer = std::shared_ptr<Camera>;
    class Buffer;
    using BufferPointer = std::shared_ptr<Buffer>;
    class PipelineState;
    using PipelineStatePointer = std::shared_ptr<PipelineState>;

    class PrimitiveDrawable;

    struct VISUALIZATION_API PrimitiveDrawableUniforms {
        int numNodes{ 0 };
    };
    using PrimitiveDrawableUniformsPointer = std::shared_ptr<PrimitiveDrawableUniforms>;

    class VISUALIZATION_API PrimitiveDrawableFactory {
    public:
        PrimitiveDrawableFactory();
        ~PrimitiveDrawableFactory();

        // Cache the shaders and pipeline to share them accross multiple instances of drawcalls
        void allocateGPUShared(const graphics::DevicePointer& device);

        // Create PrimitiveDrawable for a given Primitive document
        graphics::PrimitiveDrawable* createPrimitive(const graphics::DevicePointer& device);

        // Create Drawcall object drawing the PrimitiveDrawable in the rendering context
        void allocateDrawcallObject(
            const graphics::DevicePointer& device,
            const graphics::ScenePointer& scene,
            const graphics::CameraPointer& camera,
            graphics::PrimitiveDrawable& primitive);

        // Read / write shared uniforms
        const PrimitiveDrawableUniforms& getUniforms() const { return (*_sharedUniforms); }
        PrimitiveDrawableUniforms& editUniforms() { return (*_sharedUniforms); }

    protected:
        PrimitiveDrawableUniformsPointer _sharedUniforms;
        graphics::PipelineStatePointer _primitivePipeline;
    };
    using PrimitiveDrawableFactoryPointer = std::shared_ptr< PrimitiveDrawableFactory>;


    class VISUALIZATION_API PrimitiveDrawable {
    public:

        void swapUniforms(const PrimitiveDrawableUniformsPointer& uniforms) { _uniforms = uniforms; }
        const PrimitiveDrawableUniformsPointer& getUniforms() const { return _uniforms; }

        core::vec3 _size{ 1.0f };
        core::aabox3 getBound() const { return core::aabox3(); }
        DrawObjectCallback getDrawcall() const { return _drawcall; }

    protected:
        friend class PrimitiveDrawableFactory;

        PrimitiveDrawableUniformsPointer _uniforms;
        DrawObjectCallback _drawcall;
    };

} // !namespace graphics
