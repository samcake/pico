// Sky.h 
//
// Sam Gateau - January 2022
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

#include <mutex>

#include "render.h"
#include "Transform.h"

namespace graphics {

    struct Atmosphere {
        float earthRadius{ 6360e3 };                         // In the paper this is usually Rg or Re (radius ground, eart) 
        float atmosphereRadius{ 6420e3 };                    // In the paper this is usually R or Ra (radius atmosphere) 
        float Hr{ 7994 };                                    // Thickness of the atmosphere if density was uniform (Hr) 
        float Hm{ 1200 };                                    // Same as above but for Mie scattering (Hm) 

        float4 betaR{ 5.8e-6f, 13.5e-6f, 33.1e-6f, 0 };     // Rayleygh Scattering
        float4 betaM{ 4e-6f };                             // Mie Scattering 
        // Article implementation : (3.8e-6f, 13.5e-6f, 33.1e-6f); and (2e-6f)
    };

    struct SphericalHarmonics {
        static const int32_t DIM = 9;
        float4 coefs[DIM];
    };

    struct VISUALIZATION_API SkyData {
        Atmosphere _atmosphere;
        float3 _sunDirection = normalize(float3(0, 1.0, 1.0));
        float _sunIntensity = 10.0;
        core::mat4x3 _stageRT = core::translation(float3(0,0,0));
        core::ivec4 _simDim = { 16, 8 , 1024, 1024};
        core::ivec4 _drawControl = { 0, 0, 0, 0 };

        SphericalHarmonics _sh;
    };

    class VISUALIZATION_API Sky {
        struct SkyCPU {
            uint32_t _version{ 0 };
            mutable std::mutex _access;
            SkyData _data;
        };

        struct SkyGPU {
            uint32_t _version{ 0xFFFFFFFF };
            mutable std::mutex _access;
            BufferPointer _buffer;
        };

        SkyCPU _cpuData;
        SkyGPU _gpuData;

        TexturePointer _skymap;
        uint32_t _skymapVersion{ 0xFFFFFFFF };

    public:
        Sky();
        ~Sky();

        // Sun direction
        void setSunDir(const float3& dir);
        float3 getSunDir() const;

        void setStage(const core::mat4x3& mat);
        core::mat4x3 getStage() const;

        void setStageAltitude(float alt);
        float getStageAltitude() const;

        void setSimDim(const int4& dims);
        int4 getSimDim() const;

        // Gpu version of the Sky Data
        void allocateGPUData(const DevicePointer& device);
        bool updateGPUData();
        BufferPointer getGPUBuffer() const;

        // Skymap is generated by SkyDraw but kept here
        bool needSkymapUpdate() const;
        void resetNeedSkymapUpdate();

        TexturePointer getSkymap() const;

        uint32_t getIrradianceSHOffsetInGPUBuffer() const;

        bool isDebugEnabled() const;
        void setDebugEnabled(bool enabled);

    };
}
