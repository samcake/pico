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


    // Utility mesh connectivity information:
    using ModelUVSeam = uint32_t;
    using ModelUVSeamArray = std::vector<ModelUVSeam>;
    //  generate uvmap seams edge list
    ModelUVSeamArray computeUVSeamsEdges(const ModelDrawable& model);


    class ModelDrawableInspector;
    class ModelDrawableInspectorPart;

    struct VISUALIZATION_API ModelDrawableInspectorUniforms {
        int32_t numNodes{ 0 };
        int32_t numParts{ 0 };
        int32_t numTriangles{ 0 };
        int32_t numMaterials{ 0 };
        int32_t numEdges{ 0 };

        bool showUVMeshOutside{false};
        bool showUVMeshEdges{ false };
        bool showUVMeshFaces{ false };
        bool showUVMeshFacesID{ false };

        bool showUVGrid{ false };
        bool linearSampler{ false };
        bool lightShading{ false };

        bool render3DModel{ true };
        bool renderWireframe{ false };
        bool renderUVEdgeLines{ false };
        bool renderConnectivity{ false };
        bool renderKernelSamples{ false };

        bool renderUVMeshPoints{ false };
        bool renderUVSpace{ true };
  
        int32_t inspectedTexelX{ 0 };
        int32_t inspectedTexelY{ 0 };

        int32_t mapWidth{ 0 };
        int32_t mapHeight{ 0 };

        int32_t inspectedTriangle{ -1 };
        int32_t numInspectedTriangles{ 1 };

        uint8_t inspectedMap{ 0 }; // 0 for albedo, 1 for normal ...

        uint8_t displayedColor{ 0 }; // 0 for albedo, 1 for normal ...

        bool makeUVMeshMap{ true };
        bool uvmeshEdgeLinesPass{ true };

        bool makeComputedMap{ true };

        float uvSpaceCenterX = 0.0f;
        float uvSpaceCenterY = 0.0f;
        float uvSpaceScale = 1.0f;

        float colorMapBlend = 0.5f;

        enum {
            FKT_IMAGE_SPACE = 0,
            FKT_MESH_SPACE = 1,
        };
        uint8_t filterKernelTechnique{ FKT_MESH_SPACE }; // 0 for image space, 1 for 3d space ...

        bool maskOutsideUV{ true };
        float kernelRadius = 0.1f;
        float kernelRadiusMax = 1.0f;

        int32_t numKernelSamples{ 16 };

        void setFilterKernelTechnique(uint8_t ftk) {
            if (filterKernelTechnique != ftk) {
                filterKernelTechnique = ftk;
                if (filterKernelTechnique == graphics::ModelDrawableInspectorUniforms::FKT_IMAGE_SPACE) {
                    kernelRadius = 5.f;
                    kernelRadiusMax = 10.0f;
                }
                else {
                    kernelRadius = 0.1f;
                    kernelRadiusMax = 1.0f;
                }
            }
        }
        
        enum {
            // first bit is used internally
            SHOW_UVMESH_OUTSIDE_BIT = 0x00000001,
            SHOW_UVMESH_FACES_BIT = 0x00000002,
            SHOW_UVMESH_EDGES_BIT = 0x00000004,
            SHOW_UVMESH_FACES_ID_BIT = 0x00000008,

            SHOW_UV_GRID_BIT = 0x00000010,
            MASK_OUTSIDE_UV_BIT = 0x00000020,
            LINEAR_SAMPLER_BIT = 0x00000040,
            LIGHT_SHADING_BIT = 0x00000080,

            MAKE_UVMESH_MAP_BIT = 0x00000100,
            RENDER_UV_SPACE_BIT = 0x00000200,
            RENDER_UV_EDGE_LINES_BIT = 0x00000400,
            RENDER_WIREFRAME_BIT = 0x00000800,

            INSPECTED_MAP_BITS = 0x000F0000,
            INSPECTED_MAP_OFFSET = 16,
            DISPLAYED_COLOR_BITS = 0x00F00000,
            DISPLAYED_COLOR_OFFSET = 20,
        };
        uint32_t buildFlags() const {
            return (uint32_t)
                  (showUVMeshOutside)*SHOW_UVMESH_OUTSIDE_BIT
                | (showUVMeshFaces)*SHOW_UVMESH_FACES_BIT
                | (showUVMeshEdges)*SHOW_UVMESH_EDGES_BIT
                | (showUVMeshFacesID)*SHOW_UVMESH_FACES_ID_BIT

                | (showUVGrid)*SHOW_UV_GRID_BIT
                | (maskOutsideUV)*MASK_OUTSIDE_UV_BIT
                | (linearSampler)*LINEAR_SAMPLER_BIT
                | (lightShading)*LIGHT_SHADING_BIT

                |(renderUVSpace)*RENDER_UV_SPACE_BIT

                | (INSPECTED_MAP_BITS & (inspectedMap << INSPECTED_MAP_OFFSET))
                | (DISPLAYED_COLOR_BITS & (displayedColor << DISPLAYED_COLOR_OFFSET))
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
        ModelDrawableInspectorUniformsPointer getUniformsPtr() const { return _sharedUniforms; }

    protected:
        ModelDrawableInspectorUniformsPointer _sharedUniforms;

        graphics::PipelineStatePointer _pipeline_draw_mesh;
        graphics::PipelineStatePointer _pipeline_draw_edges;
        graphics::PipelineStatePointer _pipeline_draw_connectivity;
        graphics::PipelineStatePointer _pipeline_draw_kernelSamples;

        graphics::PixelFormat _uvmeshMapFormat = graphics::PixelFormat::R32G32B32A32_FLOAT;
        graphics::PipelineStatePointer _pipeline_uvmesh_makeEdge;
        graphics::PipelineStatePointer _pipeline_uvmesh_makeFace;


        graphics::PipelineStatePointer _pipeline_draw_uvspace_inspect;

        graphics::PipelineStatePointer _pipeline_draw_uvmesh_point;

        graphics::PipelineStatePointer _pipeline_compute_imageSpaceBlur;
        graphics::PipelineStatePointer _pipeline_compute_meshSpaceBlur;
    };
    using ModelDrawableInspectorFactoryPointer = std::shared_ptr< ModelDrawableInspectorFactory>;


    class VISUALIZATION_API ModelDrawableInspector : public ModelDrawable {
    public:

        void swapUniforms(const ModelDrawableInspectorUniformsPointer& uniforms) { _uniforms = uniforms; }
        const ModelDrawableInspectorUniformsPointer& getUniforms() const { return _uniforms; }
        
        // PRE pass DrawbleID
        DrawableID _pre_pass_ID;

        // POST pass DrawbleID
        DrawableID _post_pass_ID;


    protected:
        friend class ModelDrawableInspectorFactory;

        const ModelDrawable* _inspectedModelDrawable;

        graphics::BufferPointer _buffer_edge;

        graphics::TexturePointer _texture_uvmesh;
        graphics::FramebufferPointer _framebuffer_uvmesh;

        graphics::DescriptorSetPointer  _descriptorSet_uvmesh;
        graphics::DescriptorSetPointer _descriptorSet_compute;

        graphics::TexturePointer _texture_compute;

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


} // !namespace graphics
