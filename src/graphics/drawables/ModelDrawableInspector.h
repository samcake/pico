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


    // Utility: generate uvmap seams edge list
    struct ModelEdge {
        uint32_t p;
        uint32_t i0;
        uint32_t i1;
        uint32_t d;
    };
    using ModelEdgeArray = std::vector<ModelEdge>;
    ModelEdgeArray computeUVSeamsEdges(const ModelDrawable& model);


    class ModelDrawableInspector;
    class ModelDrawableInspectorPart;

    struct VISUALIZATION_API ModelDrawableInspectorUniforms {
        int numNodes{ 0 };
        bool showUVGrid{ false };
        bool showUVEdgeTexels{ false };
        bool showUVMesh{ true };

        bool showUVEdges{ false };

        bool render3DModel{ false };
        bool renderUVSpace{ false };
  

        bool renderMakeUVEdgeMap{ true };
        bool runFilter{ true };
        bool maskOutsideUV{ true };
        bool linearSampler{ false };

        float uvSpaceCenterX = 0.0f;
        float uvSpaceCenterY = 0.0f;
        float uvSpaceScale = 1.0f;
        float colorMapBlend = 0.5f;

        float kernelRadius = 5.0f;

 
        void makeUVEdgeMap() {
            renderMakeUVEdgeMap = true;
        }
        void doRunFilter() {
            runFilter = true;
        }
        
        enum {
            // first bit is used internally
            RENDER_UV_SPACE_BIT = 0x00000001,
            SHOW_UV_MESH_BIT = 0x00000002,
            SHOW_UV_EDGE_TEXELS_BIT = 0x00000004,
            SHOW_UV_GRID_BIT = 0x00000008,

            DRAW_EDGE_LINE_BIT = 0x00000010,
            MASK_OUTSIDE_UV_BIT = 0x00000020,
            LINEAR_SAMPLER_BIT = 0x00000040,

            MAKE_EDGE_MAP_BIT = 0x00000100,

        };
        uint32_t buildFlags() const {
            return (uint32_t)
                (renderUVSpace)*RENDER_UV_SPACE_BIT
                | (showUVMesh)*SHOW_UV_MESH_BIT
                | (showUVEdgeTexels)*SHOW_UV_EDGE_TEXELS_BIT
                | (showUVGrid)*SHOW_UV_GRID_BIT

                | (maskOutsideUV)*MASK_OUTSIDE_UV_BIT
                | (linearSampler)*LINEAR_SAMPLER_BIT
                ;
        }
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
        graphics::PipelineStatePointer _pipelineInspectUVMap;

        graphics::PipelineStatePointer _pipelineMakeSeamMap_edge;
        graphics::PipelineStatePointer _pipelineMakeSeamMap_face;

        graphics::PipelineStatePointer _pipelineInspectUVSpace;


        graphics::PipelineStatePointer _pipelineUVSpaceComputeBlur;
    };
    using ModelDrawableInspectorFactoryPointer = std::shared_ptr< ModelDrawableInspectorFactory>;


    class VISUALIZATION_API ModelDrawableInspector : public ModelDrawable {
    public:

        void swapUniforms(const ModelDrawableInspectorUniformsPointer& uniforms) { _uniforms = uniforms; }
        const ModelDrawableInspectorUniformsPointer& getUniforms() const { return _uniforms; }
        
        graphics::BufferPointer getEdgeBuffer() const { return _edgeBuffer; }

        ModelEdgeArray _edges;

        // Edges DrawableID
        DrawableID _drawEdgesID;

        // Make edges DrawbleID
        DrawableID _makeEdgesID;

    protected:
        friend class ModelDrawableInspectorFactory;

        const ModelDrawable* _inspectedModelDrawable;

        graphics::BufferPointer _edgeBuffer;

        graphics::TexturePointer _edgeTexture;
        graphics::FramebufferPointer _edgeFramebuffer;

        graphics::DescriptorSetPointer  _descriptorSet_makeEdgeMap;

        graphics::DescriptorSetPointer _descriptorSet_compute;

        graphics::TexturePointer _computeTexture;

        ModelDrawableInspectorUniformsPointer _uniforms;
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
    
    class VISUALIZATION_API ModelDrawableInspectorEdges {
    public:
        core::aabox3 getBound() const { return _bound; }
        DrawObjectCallback getDrawcall() const { return _drawcall; }


    protected:
        friend class ModelDrawableInspectorFactory;
        DrawObjectCallback _drawcall;
        core::aabox3 _bound;
    };

} // !namespace graphics
