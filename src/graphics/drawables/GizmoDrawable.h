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

#include <render/Scene.h>
#include <render/Drawable.h>

namespace graphics {
    class Device;
    using DevicePointer = std::shared_ptr<Device>;
    class TransformTreeGPU;
    using TransformTreeGPUPointer = std::shared_ptr<TransformTreeGPU>;
    class Camera;
    using CameraPointer = std::shared_ptr<Camera>;
    class Buffer;
    using BufferPointer = std::shared_ptr<Buffer>;
    class PipelineState;
    using PipelineStatePointer = std::shared_ptr<PipelineState>;

    class NodeGizmo;
    class ItemGizmo;

    struct VISUALIZATION_API GizmoDrawableUniforms {
        float triangleScale{ 0.05f };
        int numNodes{ 0 };
    };
    using GizmoDrawableUniformsPointer = std::shared_ptr<GizmoDrawableUniforms>;

    class VISUALIZATION_API GizmoDrawableFactory {
    public:
        GizmoDrawableFactory();
        ~GizmoDrawableFactory();

        // Cache the shaders and pipeline to share them accross multiple instances of drawcalls
        void allocateGPUShared(const graphics::DevicePointer& device);

        // Create GizmoDrawable for a given Gizmo document, building the gpu vertex buffer
        graphics::NodeGizmo* createNodeGizmo(const graphics::DevicePointer& device);
        graphics::ItemGizmo* createItemGizmo(const graphics::DevicePointer& device);

        // Create Drawcall object drawing the GizmoDrawable in the rendering context
        void allocateDrawcallObject(
            const graphics::DevicePointer& device,
            const graphics::ScenePointer& scene,
            const graphics::CameraPointer& camera,
            graphics::NodeGizmo& gizmo);
        void allocateDrawcallObject(
            const graphics::DevicePointer& device,
            const graphics::ScenePointer& scene,
            const graphics::CameraPointer& camera,
            graphics::ItemGizmo& gizmo);

        // Read / write shared uniforms
        const GizmoDrawableUniforms& getUniforms() const { return (*_sharedUniforms); }
        GizmoDrawableUniforms& editUniforms() { return (*_sharedUniforms); }

    protected:
        GizmoDrawableUniformsPointer _sharedUniforms;
        graphics::PipelineStatePointer _nodePipeline;
        graphics::PipelineStatePointer _itemPipeline;
    };
    using GizmoDrawableFactoryPointer = std::shared_ptr< GizmoDrawableFactory>;


    class VISUALIZATION_API NodeGizmo {
    public:
        std::vector<NodeID> nodes; 

        void swapUniforms(const GizmoDrawableUniformsPointer& uniforms) { _uniforms = uniforms; }
        const GizmoDrawableUniformsPointer& getUniforms() const { return _uniforms; }

        core::aabox3 getBound() const { return core::aabox3(); }
        DrawObjectCallback getDrawcall() const { return _drawcall; }

    protected:
        friend class GizmoDrawableFactory;

        GizmoDrawableUniformsPointer _uniforms;
        DrawObjectCallback _drawcall;
    };

    class VISUALIZATION_API ItemGizmo {
    public:
        std::vector<ItemID> items;

        void swapUniforms(const GizmoDrawableUniformsPointer& uniforms) { _uniforms = uniforms; }
        const GizmoDrawableUniformsPointer& getUniforms() const { return _uniforms; }

        core::aabox3 getBound() const { return core::aabox3(); }
        DrawObjectCallback getDrawcall() const { return _drawcall; }

    protected:
        friend class GizmoDrawableFactory;

        GizmoDrawableUniformsPointer _uniforms;
        DrawObjectCallback _drawcall;
    };


} // !namespace graphics
