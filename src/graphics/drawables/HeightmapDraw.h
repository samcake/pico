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
#include <core/math/Math3D.h>
#include "dllmain.h"

#include <render/Scene.h>
#include <render/Draw.h>

namespace graphics {
    class Device;
    using DevicePointer = std::shared_ptr<Device>;
    class Camera;
    using CameraPointer = std::shared_ptr<Camera>;
    class Buffer;
    using BufferPointer = std::shared_ptr<Buffer>;
    class PipelineState;
    using PipelineStatePointer = std::shared_ptr<PipelineState>;

    class HeightmapDraw;

    struct Heightmap {
        uint32_t map_width{ 1 };
        uint32_t map_height{ 1 };
        float    map_spacing{ 1.f };

        uint32_t getMapNumElementsX() const { return map_width; }
        uint32_t getMapNumElementsY() const { return map_height; }
        uint32_t getMapNumElements() const { return getMapNumElementsX() * getMapNumElementsY(); }

        uint32_t mesh_resolutionX{ 1 };
        uint32_t mesh_resolutionY{ 1 };
        float    mesh_spacing{ 1.f };
    
        uint32_t getMeshNumVertsX() const { return mesh_resolutionX + 1; }
        uint32_t getMeshNumVertsY() const { return mesh_resolutionY + 1; }
        uint32_t getMeshNumVerts() const {  return getMeshNumVertsX() * getMeshNumVertsY(); }

        uint32_t getMeshPerStripNumIndices() const { return (2 * mesh_resolutionX) + 1; }
        uint32_t getMeshNumStrips() const { return mesh_resolutionY; }
        uint32_t getMeshNumIndices() const { return getMeshNumStrips() * getMeshPerStripNumIndices() - 1; } // remove the last index of the last strip
 
        std::vector<float> heights;

    };

    struct VISUALIZATION_API HeightmapDrawUniforms {
    };
    using HeightmapDrawUniformsPointer = std::shared_ptr<HeightmapDrawUniforms>;

    class VISUALIZATION_API HeightmapDrawFactory {
    public:
        HeightmapDrawFactory(const graphics::DevicePointer& device);
        ~HeightmapDrawFactory();

        // Create HeightmapDraw for a given Heightmap document
        graphics::HeightmapDraw createHeightmap(const graphics::DevicePointer& device, const Heightmap& heightmap);

        // Read / write shared uniforms
        const HeightmapDrawUniforms& getUniforms() const { return (*_sharedUniforms); }
        HeightmapDrawUniforms& editUniforms() { return (*_sharedUniforms); }

    protected:
        HeightmapDrawUniformsPointer _sharedUniforms;
        graphics::PipelineStatePointer _HeightmapPipeline;

        graphics::PipelineStatePointer _computePipeline;

        // Cache the shaders and pipeline to share them accross multiple instances of drawcalls
        void allocateGPUShared(const graphics::DevicePointer& device);

        // Create Drawcall object drawing the HeightmapDraw in the rendering context
        void allocateDrawcallObject(const graphics::DevicePointer& device, graphics::HeightmapDraw& Heightmap);
    };
    using HeightmapDrawFactoryPointer = std::shared_ptr< HeightmapDrawFactory>;


    struct VISUALIZATION_API HeightmapDraw {
    public:
        const HeightmapDrawUniformsPointer& getUniforms() const { return _uniforms; }

        core::aabox3 getBound() const { return core::aabox3(core::vec3(0.f), core::vec3(_heightmap.map_width, 2.0f, _heightmap.map_height) * 0.5f *_heightmap.map_spacing); }
        DrawObjectCallback getDrawcall() const { return _drawcall; }

        graphics::BufferPointer getHeightBuffer() const { return _heightBuffer; }

        const Heightmap& getDesc() const { return _heightmap; }

    protected:
        friend class HeightmapDrawFactory;
        Heightmap _heightmap;
        graphics::BufferPointer _heightBuffer;
        HeightmapDrawUniformsPointer _uniforms;
        DrawObjectCallback _drawcall;

    };

} // !namespace graphics
