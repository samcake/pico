// GizmoDraw.h 
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
#include <render/Draw.h>

namespace graphics {
    class Device;
    using DevicePointer = std::shared_ptr<Device>;
    class TransformTreeGPU;
    using TransformTreeGPUPointer = std::shared_ptr<TransformTreeGPU>;
    class Buffer;
    using BufferPointer = std::shared_ptr<Buffer>;
    class PipelineState;
    using PipelineStatePointer = std::shared_ptr<PipelineState>;

    class NodeGizmo;
    class ItemGizmo;
    class CameraGizmo;

    struct VISUALIZATION_API GizmoDrawUniforms {
        uint32_t  indexOffset{ 0 };
        uint32_t  indexCount{ uint32_t(-1)}; // -1 means we are going to draw from offset until the end of the scene store
        bool showTransform{ true };
        bool showBranch{ true };
        bool showLocalBound{ true };
        bool showWorldBound{ false };

        enum {
            SHOW_TRANSFORM_BIT = 0x00000001,
            SHOW_BRANCH_BIT = 0x00000002,
            SHOW_LOCAL_BOUND_BIT = 0x00000004,
            SHOW_WORLD_BOUND_BIT = 0x00000008,
        };
        uint32_t buildFlags() const;
    };
    using GizmoDrawUniformsPointer = std::shared_ptr<GizmoDrawUniforms>;

    class VISUALIZATION_API GizmoDrawFactory {
        // Cache the shaders and pipeline to share them accross multiple instances of drawcalls
        void allocateGPUShared(const graphics::DevicePointer& device);

    public:
        GizmoDrawFactory(const graphics::DevicePointer& device);
        ~GizmoDrawFactory();

        // Create GizmoDraw for a given Gizmo document, building the gpu vertex buffer
        graphics::NodeGizmo* createNodeGizmo(const graphics::DevicePointer& device);
        graphics::ItemGizmo* createItemGizmo(const graphics::DevicePointer& device);
        graphics::CameraGizmo* createCameraGizmo(const graphics::DevicePointer& device);

        // Create Drawcall object drawing the GizmoDraw in the rendering context
        void allocateDrawcallObject(
            const graphics::DevicePointer& device,
            const graphics::ScenePointer& scene,
            graphics::NodeGizmo& gizmo);
        void allocateDrawcallObject(
            const graphics::DevicePointer& device,
            const graphics::ScenePointer& scene,
            graphics::ItemGizmo& gizmo);
        void allocateDrawcallObject(
            const graphics::DevicePointer& device,
            const graphics::ScenePointer& scene,
            graphics::CameraGizmo& gizmo);

        // Read / write shared uniforms
        const GizmoDrawUniforms& getUniforms() const { return (*_sharedUniforms); }
        GizmoDrawUniforms& editUniforms() { return (*_sharedUniforms); }

    protected:
        GizmoDrawUniformsPointer _sharedUniforms;
        graphics::PipelineStatePointer _nodePipeline;
        graphics::PipelineStatePointer _itemPipeline;
        graphics::PipelineStatePointer _cameraPipeline;
    };
    using GizmoDrawFactoryPointer = std::shared_ptr< GizmoDrawFactory>;


    class VISUALIZATION_API NodeGizmo {
    public:
        void swapUniforms(const GizmoDrawUniformsPointer& uniforms) { _uniforms = uniforms; }
        const GizmoDrawUniformsPointer& getUniforms() const { return _uniforms; }

        core::aabox3 getBound() const { return core::aabox3(); }
        DrawObjectCallback getDrawcall() const { return _drawcall; }

    protected:
        friend class GizmoDrawFactory;

        GizmoDrawUniformsPointer _uniforms;
        DrawObjectCallback _drawcall;
    };

    class VISUALIZATION_API ItemGizmo {
    public:

        void swapUniforms(const GizmoDrawUniformsPointer& uniforms) { _uniforms = uniforms; }
        const GizmoDrawUniformsPointer& getUniforms() const { return _uniforms; }

        core::aabox3 getBound() const { return core::aabox3(); }
        DrawObjectCallback getDrawcall() const { return _drawcall; }

    protected:
        friend class GizmoDrawFactory;

        GizmoDrawUniformsPointer _uniforms;
        DrawObjectCallback _drawcall;
    };

    class VISUALIZATION_API CameraGizmo {
    public:

        void swapUniforms(const GizmoDrawUniformsPointer& uniforms) { _uniforms = uniforms; }
        const GizmoDrawUniformsPointer& getUniforms() const { return _uniforms; }

        core::aabox3 getBound() const { return core::aabox3(); }
        DrawObjectCallback getDrawcall() const { return _drawcall; }

    protected:
        friend class GizmoDrawFactory;

        GizmoDrawUniformsPointer _uniforms;
        DrawObjectCallback _drawcall;
    };

    std::tuple<graphics::Item, graphics::Item> GizmoDraw_createSceneGizmos(const ScenePointer& scene, const DevicePointer& gpudevice);

} // !namespace graphics
