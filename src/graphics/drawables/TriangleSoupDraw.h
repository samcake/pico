// TriangleSoupDraw.h 
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
#include <core/math/Math3D.h>
#include "dllmain.h"
#include <render/Scene.h>
#include <render/Draw.h>

namespace document {
    class TriangleSoup;
    using TriangleSoupPointer = std::shared_ptr<TriangleSoup>;
}
namespace graphics {
    class Device;
    using DevicePointer = std::shared_ptr<Device>;
    class Camera;
    using CameraPointer = std::shared_ptr<Camera>;
    class Buffer;
    using BufferPointer = std::shared_ptr<Buffer>;
    class PipelineState;
    using PipelineStatePointer = std::shared_ptr<PipelineState>;

    class TriangleSoupDraw;
    using TriangleSoupDrawPointer = std::shared_ptr<TriangleSoupDraw>;

    struct VISUALIZATION_API TriangleSoupDrawUniforms {
        float triangleScale{ 0.05f };
    };
    using TriangleSoupDrawUniformsPointer = std::shared_ptr<TriangleSoupDrawUniforms>;

    class VISUALIZATION_API TriangleSoupDrawFactory {
    public:
        TriangleSoupDrawFactory(const graphics::DevicePointer& device);
        ~TriangleSoupDrawFactory();


        // Create TriangleSoupDraw for a given TriangleSoup document, building the gpu resource
        graphics::TriangleSoupDraw createTriangleSoupDraw(const graphics::DevicePointer& device, const document::TriangleSoupPointer& pointcloud);


        // Read / write shared uniforms
        const TriangleSoupDrawUniforms& getUniforms() const { return (*_sharedUniforms); }
        TriangleSoupDrawUniforms& editUniforms() { return (*_sharedUniforms); }

    protected:
        TriangleSoupDrawUniformsPointer _sharedUniforms;
        graphics::PipelineStatePointer _pipeline;

        // Cache the shaders and pipeline to share them accross multiple instances of drawcalls
        void allocateGPUShared(const graphics::DevicePointer& device);

        // Create Drawcall object drawing the TriangleSoupDraw in the rendering context
        void allocateDrawcallObject(const graphics::DevicePointer& device, graphics::TriangleSoupDraw& pointcloudDraw);

    };
    using TriangleSoupDrawFactoryPointer = std::shared_ptr< TriangleSoupDrawFactory>;



    struct VISUALIZATION_API TriangleSoupDraw {
    public:
        const TriangleSoupDrawUniformsPointer& getUniforms() const { return _uniforms; }

        graphics::BufferPointer getVertexBuffer() const { return _vertexBuffer; }
        graphics::BufferPointer getIndexBuffer() const { return _indexBuffer; }

        core::aabox3 getBound() const { return _bounds.toBox(); }
        DrawObjectCallback getDrawcall() const { return _drawcall; }

    protected:
        friend class TriangleSoupDrawFactory;

        TriangleSoupDrawUniformsPointer _uniforms;
        graphics::BufferPointer _vertexBuffer;
        graphics::BufferPointer _indexBuffer;
        core::Bounds _bounds;
        DrawObjectCallback _drawcall;

    };


} // !namespace graphics
