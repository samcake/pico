// PointCloud_Draw.h 
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

#include <memory>
#include <core/math/LinearAlgebra.h>
#include "dllmain.h"
#include <render/Scene.h>
#include <render/Draw.h>

namespace document {
    class PointCloud;
    using PointCloudPointer = std::shared_ptr<PointCloud>;
}
namespace graphics {
    class Device;
    using DevicePointer = std::shared_ptr<Device>;
    class Camera;
    using CameraPointer = std::shared_ptr<Camera>;
    class PointCloudDraw;
    using PointCloudDrawPointer = std::shared_ptr<PointCloudDraw>;
    class Buffer;
    using BufferPointer = std::shared_ptr<Buffer>;
    class PipelineState;
    using PipelineStatePointer = std::shared_ptr<PipelineState>;

    struct VISUALIZATION_API PointCloudDrawUniforms {
        // Size of the sprite expressed in pixels
        float spriteSize{ 1.0f }; 

        // control if the point sprites are rendered perspective correct divided by the depth (1.0)
        // or fixed size in pixels (0.0)
        float perspectiveSprite{ 0.0f };

        // In perspective mode, depth where the sprite is exactly at spriteSize
        float perspectiveDepth{ 1.0f };

        // For debug, draw points at the perspective plane with a control color
        float showPerspectiveDepthPlane{ 0.0f };

        void setSpriteSize(float v) { spriteSize = core::clamp(v, 0.01f, 5.0f); }
        float getSpriteSize() const { return spriteSize; }

        void setPerspectiveSprite(float v) { perspectiveSprite =  core::clamp(v, 0.0f, 1.0f); }
        float getPerspectiveSprite() const { return perspectiveSprite; }

        void setPerspectiveDepth(float v) { perspectiveDepth = core::max(0.01f, v); }
        float getPerspectiveDepth() const { return perspectiveDepth; }

        void setShowPerspectiveDepthPlane(float v) { showPerspectiveDepthPlane = core::clamp(v, 0.0f, 1.0f); }
        float getShowPerspectiveDepthPlane() const { return showPerspectiveDepthPlane; }
    };
    using PointCloudDrawUniformsPointer = std::shared_ptr<PointCloudDrawUniforms>;

    class VISUALIZATION_API PointCloudDrawFactory {
    public:
        PointCloudDrawFactory(const graphics::DevicePointer& device);
        ~PointCloudDrawFactory();

        // Create PointCloudDraw for a given PointCloud document, building the gpu vertex buffer
        graphics::PointCloudDraw createPointCloudDraw(const graphics::DevicePointer& device, const document::PointCloudPointer& pointcloud);
 
        // Read / write shared uniforms
        const PointCloudDrawUniforms& getUniforms() const { return (*_sharedUniforms); }
        PointCloudDrawUniforms& editUniforms() { return (*_sharedUniforms); }
        
    protected:
        PointCloudDrawUniformsPointer _sharedUniforms;
        graphics::PipelineStatePointer _pipeline;

        // Cache the shaders and pipeline to share them accross multiple instances of drawcalls
        void allocateGPUShared(const graphics::DevicePointer& device);

        // Create Drawcall object drawing the PointCloudDraw in the rendering context
        void allocateDrawcallObject(const graphics::DevicePointer& device, graphics::PointCloudDraw& pointcloudDraw);
    };
    using PointCloudDrawFactoryPointer = std::shared_ptr< PointCloudDrawFactory>;
    

    struct VISUALIZATION_API PointCloudDraw {
    public:
        const PointCloudDrawUniformsPointer& getUniforms() const { return _uniforms; }

        graphics::BufferPointer getVertexBuffer() const { return _vertexBuffer; }

        core::aabox3 getBound() const { return _bounds.toBox(); }
        DrawObjectCallback getDrawcall() const { return _drawcall; }

    protected:
        friend class PointCloudDrawFactory;

        PointCloudDrawUniformsPointer _uniforms;
        graphics::BufferPointer _vertexBuffer;
        core::mat4x3 _transform;
        core::Bounds _bounds;
        DrawObjectCallback _drawcall;
    };


} // !namespace graphics
