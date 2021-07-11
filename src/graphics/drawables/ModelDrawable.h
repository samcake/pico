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
    
    static const uint32_t MODEL_INVALID_INDEX = document::model::INVALID_INDEX;

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
        uint32_t node{ MODEL_INVALID_INDEX };
        uint32_t shape{ MODEL_INVALID_INDEX };
        uint32_t camera{ MODEL_INVALID_INDEX };
    };

    struct ModelShape {
        uint32_t partOffset{ 0 };
        uint32_t numParts{ 0 };
    };

    struct ModelPart {
        uint32_t numIndices{ 0 };
        uint32_t indexOffset{ 0 };
        uint32_t vertexOffset{ 0 };
        uint32_t attribOffset{ MODEL_INVALID_INDEX };
        uint32_t material{ MODEL_INVALID_INDEX };
        uint32_t spareA;
        uint32_t spareB;
        uint32_t spareC;
    };

    struct ModelMaterial {
        core::vec4 color{0.5f, 0.5f, 0.5f, 1.0f};
        float metallic{ 0.0f };
        float roughness{0.0f };
        float spareA;
        float spareB;
        core::vec4 emissive{ 0.0f, 0.0f, 0.0f, 1.0f };

        uint32_t baseColorTexture{ MODEL_INVALID_INDEX };
        uint32_t normalTexture{ MODEL_INVALID_INDEX };
        uint32_t rmaoTexture{ MODEL_INVALID_INDEX };
        uint32_t emissiveTexture{ MODEL_INVALID_INDEX };
    };

    struct ModelCamera {
        core::Projection _projection;
    };

    struct ModelVertex {
        float px{ 0.f };
        float py{ 0.f };
        float pz{ 0.f };
        uint32_t n{ 0 };
    };
    using ModelVertexAttrib = core::vec4;
    using ModelIndex = uint16_t;

    class VISUALIZATION_API ModelDrawable {
    public:
        virtual ~ModelDrawable() {}

        void swapUniforms(const ModelDrawableUniformsPointer& uniforms) { _uniforms = uniforms; }
        const ModelDrawableUniformsPointer& getUniforms() const { return _uniforms; }

        core::aabox3 getBound() const { return _bound; }
        DrawObjectCallback getDrawcall() const { return _drawcall; }

        // immutable buffer containing the vertices, indices, parts and materials of the model
        graphics::BufferPointer getVertexBuffer() const { return _vertexBuffer; }
        graphics::BufferPointer getVertexAttribBuffer() const { return _vertexAttribBuffer; }
        graphics::BufferPointer getIndexBuffer() const { return _indexBuffer; }
        graphics::BufferPointer getPartBuffer() const { return _partBuffer; }

        graphics::BufferPointer getMaterialBuffer() const { return _materialBuffer; }
        graphics::TexturePointer getAlbedoTexture() const { return _albedoTexture; }

        std::vector<ModelVertex> _vertices;
        std::vector<ModelVertexAttrib> _vertex_attribs;
        std::vector<ModelIndex> _indices;
        std::vector<ModelPart> _parts;
        std::vector<core::aabox3> _partAABBs;
        std::vector<ModelShape> _shapes;

        // For each part, we create one drawable
        DrawableIDs _partDrawables;

        // Self DrawableID
        DrawableID _drawableID;

        // Local nodes hierarchy in the model, used to create concrete instances of scene nodes when
        // instanciating a model in the scene 
        std::vector<core::mat4x3> _localNodeTransforms;
        std::vector<uint32_t> _localNodeParents;

        // Local Items used to create the list of items in the scene when instanciating the model 
        std::vector<ModelItem> _localItems;

        // local Cameras
        std::vector<ModelCamera> _localCameras;

    protected:
        friend class ModelDrawableFactory;
        friend class ModelDrawableInspectorFactory;

        graphics::BufferPointer _vertexBuffer; // core vertex attribs: pos
        graphics::BufferPointer _vertexAttribBuffer; // extra vertex attribs texcoord, ...
        graphics::BufferPointer _indexBuffer;
        graphics::BufferPointer _partBuffer;

        graphics::BufferPointer _materialBuffer;
        graphics::TexturePointer _albedoTexture;

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
