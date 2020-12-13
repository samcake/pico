// Heightmap.h 
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

    class HeightmapDrawable;

    struct Heightmap {
        uint32_t width{ 1 };
        uint32_t height{ 1 };
        float    spacing{ 1.f };
    };

    struct VISUALIZATION_API HeightmapDrawableUniforms {
    };
    using HeightmapDrawableUniformsPointer = std::shared_ptr<HeightmapDrawableUniforms>;

    class VISUALIZATION_API HeightmapDrawableFactory {
    public:
        HeightmapDrawableFactory();
        ~HeightmapDrawableFactory();

        // Cache the shaders and pipeline to share them accross multiple instances of drawcalls
        void allocateGPUShared(const graphics::DevicePointer& device);

        // Create HeightmapDrawable for a given Heightmap document
        graphics::HeightmapDrawable* createHeightmap(const graphics::DevicePointer& device, const Heightmap& heightmap);

        // Create Drawcall object drawing the HeightmapDrawable in the rendering context
        void allocateDrawcallObject(
            const graphics::DevicePointer& device,
            const graphics::ScenePointer& scene,
            const graphics::CameraPointer& camera,
            graphics::HeightmapDrawable& Heightmap);

        // Read / write shared uniforms
        const HeightmapDrawableUniforms& getUniforms() const { return (*_sharedUniforms); }
        HeightmapDrawableUniforms& editUniforms() { return (*_sharedUniforms); }

    protected:
        HeightmapDrawableUniformsPointer _sharedUniforms;
        graphics::PipelineStatePointer _HeightmapPipeline;
    };
    using HeightmapDrawableFactoryPointer = std::shared_ptr< HeightmapDrawableFactory>;


    class VISUALIZATION_API HeightmapDrawable {
    public:

        void swapUniforms(const HeightmapDrawableUniformsPointer& uniforms) { _uniforms = uniforms; }
        const HeightmapDrawableUniformsPointer& getUniforms() const { return _uniforms; }

        core::aabox3 getBound() const { return core::aabox3(core::vec3(0.f), core::vec3(_heightmap.width, 2.0f, _heightmap.height) * 0.5f *_heightmap.spacing); }
        DrawObjectCallback getDrawcall() const { return _drawcall; }

        graphics::BufferPointer getHeightBuffer() const { return _heightBuffer; }

        const Heightmap& getDesc() const { return _heightmap; }

    protected:
        friend class HeightmapDrawableFactory;
        Heightmap _heightmap;
        graphics::BufferPointer _heightBuffer;
        HeightmapDrawableUniformsPointer _uniforms;
        DrawObjectCallback _drawcall;
    };

} // !namespace graphics
