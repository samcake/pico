// GizmoDrawable.h 
//
// Sam Gateau - October 2020
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

namespace graphics {
    class DrawcallObject;
    using DrawcallObjectPointer = std::shared_ptr<DrawcallObject>;
    class Device;
    using DevicePointer = std::shared_ptr<Device>;
    class Camera;
    using CameraPointer = std::shared_ptr<Camera>;
    class Buffer;
    using BufferPointer = std::shared_ptr<Buffer>;
    class PipelineState;
    using PipelineStatePointer = std::shared_ptr<PipelineState>;

    class GizmoDrawable;
    using GizmoDrawablePointer = std::shared_ptr<GizmoDrawable>;

    struct VISUALIZATION_API GizmoDrawableUniforms {
        float triangleScale{ 0.05f };
    };
    using GizmoDrawableUniformsPointer = std::shared_ptr<GizmoDrawableUniforms>;

    class VISUALIZATION_API GizmoDrawableFactory {
    public:
        GizmoDrawableFactory();
        ~GizmoDrawableFactory();

        // Cache the shaders and pipeline to share them accross multiple instances of drawcalls
        void allocateGPUShared(const graphics::DevicePointer& device);

        // Create GizmoDrawable for a given Gizmo document, building the gpu vertex buffer
        graphics::GizmoDrawable* createGizmoDrawable(const graphics::DevicePointer& device);

        // Create Drawcall object drawing the GizmoDrawable in the rendering context
        graphics::DrawcallObjectPointer allocateDrawcallObject(const graphics::DevicePointer& device, const graphics::CameraPointer& camera,
            const graphics::GizmoDrawablePointer& pointcloudDrawable);

        // Read / write shared uniforms
        const GizmoDrawableUniforms& getUniforms() const { return (*_sharedUniforms); }
        GizmoDrawableUniforms& editUniforms() { return (*_sharedUniforms); }

    protected:
        GizmoDrawableUniformsPointer _sharedUniforms;
        graphics::PipelineStatePointer _pipeline;
    };
    using GizmoDrawableFactoryPointer = std::shared_ptr< GizmoDrawableFactory>;


    /*
    const pico::DrawcallObjectPointer& getDrawable(const GizmoDrawable& x) {
        return x.getDrawable();
    }*/
    class VISUALIZATION_API GizmoDrawable {
    public:
        GizmoDrawable();
        ~GizmoDrawable();
        
        graphics::DrawcallObjectPointer getDrawable() const;

        void swapUniforms(const GizmoDrawableUniformsPointer& uniforms) { _uniforms = uniforms; }
        const GizmoDrawableUniformsPointer& getUniforms() const { return _uniforms; }

    protected:
        friend class GizmoDrawableFactory;

        GizmoDrawableUniformsPointer _uniforms;
        graphics::DrawcallObjectPointer _drawcall;
        core::mat4x3 _transform;
        core::Bounds _bounds;
    };


} // !namespace graphics
