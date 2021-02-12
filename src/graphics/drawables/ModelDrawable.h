// Model.h 
//
// Sam Gateau - January 2021
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
#include <document/Model.h>
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

    class ModelDrawable;
    class ModelDrawablePart;

    struct VISUALIZATION_API ModelDrawableUniforms {
        int numNodes{ 0 };
    };
    using ModelDrawableUniformsPointer = std::shared_ptr<ModelDrawableUniforms>;

    class VISUALIZATION_API ModelDrawableFactory {
    public:
        ModelDrawableFactory();
        ~ModelDrawableFactory();

        // Cache the shaders and pipeline to share them accross multiple instances of drawcalls
        void allocateGPUShared(const graphics::DevicePointer& device);

        // Create ModelDrawable for a given Model document
        graphics::ModelDrawable* createModel(const graphics::DevicePointer& device, const document::ModelPointer& model);

        // Create Drawcall object drawing the ModelDrawable in the rendering context
        void allocateDrawcallObject(
            const graphics::DevicePointer& device,
            const graphics::ScenePointer& scene,
            const graphics::CameraPointer& camera,
            graphics::ModelDrawable& model);

        
        graphics::ItemIDs createModelParts(const graphics::NodeID root, const graphics::ScenePointer& scene, graphics::ModelDrawable& model);


        // Read / write shared uniforms
        const ModelDrawableUniforms& getUniforms() const { return (*_sharedUniforms); }
        ModelDrawableUniforms& editUniforms() { return (*_sharedUniforms); }

    protected:
        ModelDrawableUniformsPointer _sharedUniforms;
        graphics::PipelineStatePointer _pipeline;
    };
    using ModelDrawableFactoryPointer = std::shared_ptr< ModelDrawableFactory>;

    struct ModelItem {
        uint32_t node{ (uint32_t)-1 };
        uint32_t shape{ (uint32_t)-1 };
    };

    struct ModelShape {
        uint32_t partOffset{ 0 };
        uint32_t numParts{ 0 };
    };

    struct ModelPart {
        uint32_t numIndices{ 0 };
        uint32_t indexOffset{ 0 };
        uint32_t vertexOffset{ 0 };

        uint32_t material{ (uint32_t) -1 };
    };

    struct ModelMaterial {
        core::vec4 color{0.5f, 0.5f, 0.5f, 1.0f};
        float metallic{ 0.0f };
        float roughness{0.0f };
        float spareA;
        float spareB;
    };

    class VISUALIZATION_API ModelDrawable {
    public:

        void swapUniforms(const ModelDrawableUniformsPointer& uniforms) { _uniforms = uniforms; }
        const ModelDrawableUniformsPointer& getUniforms() const { return _uniforms; }

        core::aabox3 getBound() const { return _bound; }
        DrawObjectCallback getDrawcall() const { return _drawcall; }

        graphics::BufferPointer getVertexBuffer() const { return _vertexBuffer; }
        graphics::BufferPointer getIndexBuffer() const { return _indexBuffer; }
        graphics::BufferPointer getPartBuffer() const { return _partBuffer; }

        graphics::BufferPointer getMaterialBuffer() const { return _materialBuffer; }

        std::vector<ModelItem> _localItems;

        std::vector<core::mat4x3> _localNodeTransforms;
        std::vector<uint32_t> _localNodeParents;

        std::vector<ModelShape> _shapes;
        std::vector<ModelPart> _parts;
        std::vector<core::aabox3> _partAABBs;

        // For each part, we create one drawable
        DrawableIDs _partDrawables;

        // Self DrawableID
        DrawableID _drawableID;

    protected:
        friend class ModelDrawableFactory;

        graphics::BufferPointer _vertexBuffer;
        graphics::BufferPointer _indexBuffer;
        graphics::BufferPointer _partBuffer;

        graphics::BufferPointer _materialBuffer;

        graphics::DescriptorSetPointer  _descriptorSet;

        ModelDrawableUniformsPointer _uniforms;
        DrawObjectCallback _drawcall;
        core::aabox3 _bound;
    };

    
    class VISUALIZATION_API ModelDrawablePart {
    public:
        core::aabox3 getBound() const { return _bound; }
        DrawObjectCallback getDrawcall() const { return _drawcall; }
        
        
    protected:
        friend class ModelDrawableFactory;
        DrawObjectCallback _drawcall;
        core::aabox3 _bound;
    };
    
    

} // !namespace graphics
