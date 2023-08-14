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
#include <core/math/Math3D.h>
#include "dllmain.h"
#include <document/Model.h>
#include <render/Scene.h>
#include <render/Draw.h>
#include <render/Animation.h>

namespace graphics {
    class Device;
    using DevicePointer = std::shared_ptr<Device>;
    class Buffer;
    using BufferPointer = std::shared_ptr<Buffer>;
    class PipelineState;
    using PipelineStatePointer = std::shared_ptr<PipelineState>;

    class ModelDraw;
    class ModelDrawPart;
    
    static const uint32_t MODEL_INVALID_INDEX = document::model::INVALID_INDEX;

    struct VISUALIZATION_API ModelDrawUniforms {
        int numNodes{ 0 };
        bool lightShading{ false };
        uint8_t displayedColor{ 0 }; // 0 for albedo, 1 for normal ...

        uint32_t makeDrawMode() const;
    };
    using ModelDrawUniformsPointer = std::shared_ptr<ModelDrawUniforms>;

    class VISUALIZATION_API ModelDrawFactory {
    public:
        ModelDrawFactory(const graphics::DevicePointer& device);
        ~ModelDrawFactory();

        // Create ModelDraw for a given Model document
        graphics::ModelDraw* createModel(const graphics::DevicePointer& device, const document::ModelPointer& model);

        // Create Drawcall object drawing the ModelDraw in the rendering context
        void allocateDrawcallObject(
            const graphics::DevicePointer& device,
            const graphics::ScenePointer& scene,
            graphics::ModelDraw& model);

        graphics::ItemIDs createModelParts(const graphics::NodeID root, const graphics::ScenePointer& scene, graphics::ModelDraw& model);


        // Read / write shared uniforms
        const ModelDrawUniforms& getUniforms() const { return (*_sharedUniforms); }
        ModelDrawUniforms& editUniforms() { return (*_sharedUniforms); }
        ModelDrawUniformsPointer getUniformsPtr() const { return _sharedUniforms; }

    protected:
        ModelDrawUniformsPointer _sharedUniforms;
        graphics::PipelineStatePointer _pipeline;

        // Cache the shaders and pipeline to share them accross multiple instances of drawcalls
        void allocateGPUShared(const graphics::DevicePointer& device);
    };
    using ModelDrawFactoryPointer = std::shared_ptr< ModelDrawFactory>;

    struct ModelItem {
        uint32_t node{ MODEL_INVALID_INDEX };
        uint32_t shape{ MODEL_INVALID_INDEX };
        uint32_t skin{ MODEL_INVALID_INDEX };
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
        uint32_t numEdges{ 0 };
        uint32_t edgeOffset{ MODEL_INVALID_INDEX };
        uint32_t skinOffset{ MODEL_INVALID_INDEX };
    };

    struct ModelMaterial {
        core::vec4 color{0.5f, 0.5f, 0.5f, 1.0f};
        float metallic{ 1.0f };
        float roughness{1.0f };
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

    struct ModelVertexAttrib {
        float u{ 0.f };
        float v{ 0.f };
        uint32_t sw{ 0 }; // skin weights
        uint32_t sj{ 0 }; // skin joints
    };

    using ModelIndex = uint32_t;
    
    using ModelEdge = core::ivec4; // 2 vertex indices and 2 adjacent triangle indices (in index buffer)
    using ModelFace = core::ivec4; // 3 edge indices of the face


    struct ModelSkin {
        uint32_t jointOffset{ 0 };
        uint32_t numJoints{ 0 };
    };
    struct ModelSkinJointBinding {
        core::mat4x3 invBindingPose;
        core::ivec4  bone; // also contains the joint id in joint.x
    };

    class VISUALIZATION_API ModelDraw {
    public:
        virtual ~ModelDraw() {}

        const ModelDrawUniformsPointer& getUniforms() const { return _uniforms; }

        DrawBound getBound() const { return _bound; }
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

        // Connectivity information of the meshes
        graphics::BufferPointer getEdgeBuffer() const { return _edgeBuffer; }
        graphics::BufferPointer getFaceBuffer() const { return _faceBuffer; }
        std::vector<ModelEdge> _edges;
        std::vector<ModelFace> _faces;

        // SKinning binding info
        std::vector<ModelSkin> _skins;
        std::vector<ModelSkinJointBinding> _skinJointBindings;
        graphics::BufferPointer getSkinBuffer() const { return _skinBuffer; }

        // Utility mesh connectivity information:
        // For each part, we create one draw
        DrawIDs _partDraws;

        // Self DrawID
        DrawID _drawID;

        // Self AnimID
        AnimID _animID;

        // Local nodes hierarchy in the model, used to create concrete instances of scene nodes when
        // instanciating a model in the scene 
        std::vector<core::mat4x3> _localNodeTransforms;
        std::vector<uint32_t> _localNodeParents;
        std::vector<uint32_t> _localRootNodes;
        std::vector<std::string> _localNodeNames;

        // Local Items used to create the list of items in the scene when instanciating the model 
        std::vector<ModelItem> _localItems;

        // local Cameras
        std::vector<ModelCamera> _localCameras;

        // Ray tracing geometry
        graphics::GeometryPointer  _geometry;

        // Animations
        KeyPointer _animations;

        // the name of this model
        std::string _name;

    protected:
        friend class ModelDrawFactory;
        friend class ModelDrawInspectorFactory;

        graphics::BufferPointer _vertexBuffer; // core vertex attribs: pos
        graphics::BufferPointer _vertexAttribBuffer; // extra vertex attribs texcoord, ...
        graphics::BufferPointer _indexBuffer;
        graphics::BufferPointer _partBuffer;

        graphics::BufferPointer _edgeBuffer;
        graphics::BufferPointer _faceBuffer;
        
        graphics::BufferPointer _materialBuffer;
        graphics::TexturePointer _albedoTexture;

        graphics::BufferPointer _skinBuffer;

        graphics::DescriptorSetPointer  _descriptorSet;

        ModelDrawUniformsPointer _uniforms;
        DrawObjectCallback _drawcall;
        core::aabox3 _bound;
    };
    using ModelDrawPointer = std::shared_ptr< ModelDraw>;

    
    struct VISUALIZATION_API ModelDrawPart {
    public:
        DrawBound getBound() const { return _bound; }
        DrawObjectCallback getDrawcall() const { return _drawcall; }
        
        
    protected:
        friend class ModelDrawFactory;
        DrawObjectCallback _drawcall;
        core::aabox3 _bound;
    };
    
    

} // !namespace graphics
