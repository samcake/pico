// SkyDrawable.h 
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
#include <render/Drawable.h>
#include <gpu/Query.h>

namespace graphics {
    class Device;
    using DevicePointer = std::shared_ptr<Device>;
    class Camera;
    using CameraPointer = std::shared_ptr<Camera>;
    class Buffer;
    using BufferPointer = std::shared_ptr<Buffer>;
    class PipelineState;
    using PipelineStatePointer = std::shared_ptr<PipelineState>;

    class SkyDrawable;

    struct Atmosphere {
        core::vec3 sunDirection = core::vec3(0, 1, 0);      // The sun direction (normalized) 
        float earthRadius = 6360e3;                         // In the paper this is usually Rg or Re (radius ground, eart) 
        float atmosphereRadius = 6420e3;                    // In the paper this is usually R or Ra (radius atmosphere) 
        float Hr = 7994;                                    // Thickness of the atmosphere if density was uniform (Hr) 
        float Hm = 1200;                                    // Same as above but for Mie scattering (Hm) 

        core::vec3 betaR { 3.8e-6f, 13.5e-6f, 33.1e-6f };   // Rayleygh Scattering
        core::vec3 betaM { 21e-6f };                        // Mie Scattering
    };

    struct VISUALIZATION_API SkyDrawableUniforms {
        Atmosphere atmos;
    };

    using SkyDrawableUniformsPointer = std::shared_ptr<SkyDrawableUniforms>;

    class VISUALIZATION_API SkyDrawableFactory {
    public:
        SkyDrawableFactory();
        ~SkyDrawableFactory();

        // Cache the shaders and pipeline to share them accross multiple instances of drawcalls
        void allocateGPUShared(const graphics::DevicePointer& device);

        // Create SkyDrawable
        graphics::SkyDrawable* createDrawable(const graphics::DevicePointer& device);

        // Create Drawcall object drawing the SkyDrawable in the rendering context
        void allocateDrawcallObject(
            const graphics::DevicePointer& device,
            const graphics::ScenePointer& scene,
            graphics::SkyDrawable& primitive);

        // Read / write shared uniforms
        const SkyDrawableUniforms& getUniforms() const { return (*_sharedUniforms); }
        SkyDrawableUniforms& editUniforms() { return (*_sharedUniforms); }

    protected:
        SkyDrawableUniformsPointer _sharedUniforms;
        graphics::PipelineStatePointer _skyPipeline;
    };
    using SkyDrawableFactoryPointer = std::shared_ptr< SkyDrawableFactory>;


    class VISUALIZATION_API SkyDrawable {
    public:

        void swapUniforms(const SkyDrawableUniformsPointer& uniforms) { _uniforms = uniforms; }
        const SkyDrawableUniformsPointer& getUniforms() const { return _uniforms; }

        core::vec3 _size{ 1.0f };
        core::aabox3 getBound() const { return core::aabox3(); }
        DrawObjectCallback getDrawcall() const { return _drawcall; }

    protected:
        friend class SkyDrawableFactory;

        SkyDrawableUniformsPointer _uniforms;
        DrawObjectCallback _drawcall;
    };

} // !namespace graphics
