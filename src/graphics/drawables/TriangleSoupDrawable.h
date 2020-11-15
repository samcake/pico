// TriangleSoupDrawable.h 
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

#include <memory>
#include <core/math/LinearAlgebra.h>
#include "dllmain.h"
#include <render/Scene.h>

namespace document {
    class TriangleSoup;
    using TriangleSoupPointer = std::shared_ptr<TriangleSoup>;
}
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

    class TriangleSoupDrawable;
    using TriangleSoupDrawablePointer = std::shared_ptr<TriangleSoupDrawable>;

    struct VISUALIZATION_API TriangleSoupDrawableUniforms {
        float triangleScale{ 0.05f };
    };
    using TriangleSoupDrawableUniformsPointer = std::shared_ptr<TriangleSoupDrawableUniforms>;

    class VISUALIZATION_API TriangleSoupDrawableFactory {
    public:
        TriangleSoupDrawableFactory();
        ~TriangleSoupDrawableFactory();

        // Cache the shaders and pipeline to share them accross multiple instances of drawcalls
        void allocateGPUShared(const graphics::DevicePointer& device);

        // Create TriangleSoupDrawable for a given TriangleSoup document, building the gpu vertex buffer
        graphics::TriangleSoupDrawable* createTriangleSoupDrawable(const graphics::DevicePointer& device, const document::TriangleSoupPointer& pointcloud);

        // Create Drawcall object drawing the TriangleSoupDrawable in the rendering context
        graphics::DrawcallObjectPointer allocateDrawcallObject(const graphics::DevicePointer& device, const graphics::TransformTreeGPUPointer& transform, const graphics::CameraPointer& camera,
            const graphics::TriangleSoupDrawablePointer& pointcloudDrawable);

        // Read / write shared uniforms
        const TriangleSoupDrawableUniforms& getUniforms() const { return (*_sharedUniforms); }
        TriangleSoupDrawableUniforms& editUniforms() { return (*_sharedUniforms); }

    protected:
        TriangleSoupDrawableUniformsPointer _sharedUniforms;
        graphics::PipelineStatePointer _pipeline;
    };
    using TriangleSoupDrawableFactoryPointer = std::shared_ptr< TriangleSoupDrawableFactory>;


    /*
    const pico::DrawcallObjectPointer& getDrawable(const TriangleSoupDrawable& x) {
        return x.getDrawable();
    }*/
    class VISUALIZATION_API TriangleSoupDrawable {
    public:
        TriangleSoupDrawable();
        ~TriangleSoupDrawable();
 
        void setNode(graphics::NodeID node) const;

        graphics::DrawcallObjectPointer getDrawable() const;

        void swapUniforms(const TriangleSoupDrawableUniformsPointer& uniforms) { _uniforms = uniforms; }
        const TriangleSoupDrawableUniformsPointer& getUniforms() const { return _uniforms; }

        graphics::BufferPointer getVertexBuffer() const { return _vertexBuffer; }
        graphics::BufferPointer getIndexBuffer() const { return _indexBuffer; }


    protected:
        friend class TriangleSoupDrawableFactory;

        TriangleSoupDrawableUniformsPointer _uniforms;
        graphics::DrawcallObjectPointer _drawcall;
        graphics::BufferPointer _vertexBuffer;
        graphics::BufferPointer _indexBuffer;
        core::Bounds _bounds;
    };


} // !namespace graphics
