// DashboardDraw.h 
//
// Sam Gateau - October 2021
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
#include <gpu/Query.h>

namespace graphics {
    class Device;
    using DevicePointer = std::shared_ptr<Device>;
    class Buffer;
    using BufferPointer = std::shared_ptr<Buffer>;
    class PipelineState;
    using PipelineStatePointer = std::shared_ptr<PipelineState>;

    class DashboardDraw;

    struct VISUALIZATION_API DashboardDrawUniforms {
        int numNodes{ 0 };
    };
    using DashboardDrawUniformsPointer = std::shared_ptr<DashboardDrawUniforms>;

    class VISUALIZATION_API DashboardDrawFactory {
    public:
        DashboardDrawFactory(const graphics::DevicePointer& device);
        ~DashboardDrawFactory();

        // Create DashboardDraw
        graphics::DashboardDraw createDraw(const graphics::DevicePointer& device);

        // Read / write shared uniforms
        const DashboardDrawUniforms& getUniforms() const { return (*_sharedUniforms); }
        DashboardDrawUniforms& editUniforms() { return (*_sharedUniforms); }

    protected:
        DashboardDrawUniformsPointer _sharedUniforms;
        graphics::PipelineStatePointer _primitivePipeline;

        // Cache the shaders and pipeline to share them accross multiple instances of drawcalls
        void allocateGPUShared(const graphics::DevicePointer& device);

        // Create Drawcall object drawing the DashboardDraw in the rendering context
        void allocateDrawcallObject(const graphics::DevicePointer& device, graphics::DashboardDraw& primitive);
    };

    using DashboardDrawFactoryPointer = std::shared_ptr<DashboardDrawFactory>;


    struct VISUALIZATION_API DashboardDraw {
    public:
        const DashboardDrawUniformsPointer& getUniforms() const { return _uniforms; }

        core::vec3 _size{ 1.0f };
        core::aabox3 getBound() const { return core::aabox3(); }
        DrawObjectCallback getDrawcall() const { return _drawcall; }

    protected:
        friend class DashboardDrawFactory;

        DashboardDrawUniformsPointer _uniforms;
        DrawObjectCallback _drawcall;
    };

    Item DashboardDraw_createSceneWidgets(const ScenePointer& scene, const DevicePointer& gpuDevice);

} // !namespace graphics
