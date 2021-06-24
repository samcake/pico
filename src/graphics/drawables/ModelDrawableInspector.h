// ModelDrawableInspector.h 
//
// Sam Gateau - June 2021
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

// helper class to model Drawable
#include "ModelDrawable.h"

namespace graphics {

    class ModelDrawableInspector;
    class ModelDrawableInspectorPart;

    struct VISUALIZATION_API ModelDrawableInspectorUniforms {
        int numNodes{ 0 };
    };
    using ModelDrawableInspectorUniformsPointer = std::shared_ptr<ModelDrawableInspectorUniforms>;

    class VISUALIZATION_API ModelDrawableInspectorFactory {
    public:
        ModelDrawableInspectorFactory();
        ~ModelDrawableInspectorFactory();

        // Cache the shaders and pipeline to share them accross multiple instances of drawcalls
        void allocateGPUShared(const graphics::DevicePointer& device);

        // Create ModelDrawable for a given Model document
        graphics::ModelDrawableInspector* createModel(const graphics::DevicePointer& device, const document::ModelPointer& model, const ModelDrawable* drawable);

        // Create Drawcall object drawing the ModelDrawable in the rendering context
        void allocateDrawcallObject(
            const graphics::DevicePointer& device,
            const graphics::ScenePointer& scene,
            const graphics::CameraPointer& camera,
            graphics::ModelDrawableInspector& model);

        
        graphics::ItemIDs createModelParts(const graphics::NodeID root, const graphics::ScenePointer& scene, graphics::ModelDrawableInspector& model);


        // Read / write shared uniforms
        const ModelDrawableInspectorUniforms& getUniforms() const { return (*_sharedUniforms); }
        ModelDrawableInspectorUniforms& editUniforms() { return (*_sharedUniforms); }

    protected:
        ModelDrawableInspectorUniformsPointer _sharedUniforms;
        graphics::PipelineStatePointer _pipeline;
    };
    using ModelDrawableInspectorFactoryPointer = std::shared_ptr< ModelDrawableInspectorFactory>;


    class VISUALIZATION_API ModelDrawableInspector {
    public:

        void swapUniforms(const ModelDrawableInspectorUniformsPointer& uniforms) { _uniforms = uniforms; }
        const ModelDrawableInspectorUniformsPointer& getUniforms() const { return _uniforms; }

        core::aabox3 getBound() const { return _bound; }
        DrawObjectCallback getDrawcall() const { return _drawcall; }

        // immutable buffer containing the vertices, indices, parts and materials of the model
        graphics::BufferPointer getVertexBuffer() const { return _vertexBuffer; }
        graphics::BufferPointer getVertexAttribBuffer() const { return _vertexAttribBuffer; }
        graphics::BufferPointer getIndexBuffer() const { return _indexBuffer; }
        graphics::BufferPointer getPartBuffer() const { return _partBuffer; }

        graphics::BufferPointer getMaterialBuffer() const { return _materialBuffer; }
        graphics::TexturePointer getAlbedoTexture() const { return _albedoTexture; }

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
        friend class ModelDrawableInspectorFactory;

        const ModelDrawable* _inspectedModelDrawable;

        graphics::BufferPointer _vertexBuffer; // core vertex attribs: pos
        graphics::BufferPointer _vertexAttribBuffer; // extra vertex attribs texcoord, ...
        graphics::BufferPointer _indexBuffer;
        graphics::BufferPointer _partBuffer;

        graphics::BufferPointer _materialBuffer;
        graphics::TexturePointer _albedoTexture;

        graphics::DescriptorSetPointer  _descriptorSet;

        ModelDrawableInspectorUniformsPointer _uniforms;
        DrawObjectCallback _drawcall;
        core::aabox3 _bound;
    };

    
    class VISUALIZATION_API ModelDrawableInspectorPart {
    public:
        core::aabox3 getBound() const { return _bound; }
        DrawObjectCallback getDrawcall() const { return _drawcall; }
        
        
    protected:
        friend class ModelDrawableInspectorFactory;
        DrawObjectCallback _drawcall;
        core::aabox3 _bound;
    };
    
    

} // !namespace graphics
