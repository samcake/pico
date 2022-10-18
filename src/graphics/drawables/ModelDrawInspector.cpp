// ModelDrawInspector.cpp
//
// Sam Gateau - June 2020
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
#include "ModelDrawInspector.h"

#include "gpu/Device.h"
#include "gpu/Batch.h"
#include "gpu/Shader.h"
#include "gpu/Resource.h"
#include "gpu/Pipeline.h"
#include "gpu/Descriptor.h"
#include "gpu/Swapchain.h"
#include "gpu/Framebuffer.h"

#include "render/Renderer.h"
#include "render/Camera.h"
#include "render/Scene.h"
#include "render/Draw.h"
#include "render/Viewport.h"
#include "render/Mesh.h"

#include "Transform_inc.h"
#include "Projection_inc.h"
#include "Camera_inc.h"
#include "SceneTransform_inc.h"

#include "Mesh_inc.h"
#include "Part_inc.h"
#include "Material_inc.h"
#include "SceneModel_inc.h"

#include "Color_inc.h"
#include "Paint_inc.h"
#include "Shading_inc.h"
#include "Surface_inc.h"

#include "MeshEdge_inc.h"
#include "Triangle_inc.h"

#include "ModelInspectorPart_vert.h"
#include "ModelInspectorPart_frag.h"
#include "ModelUVSpaceProcessing_comp.h"

#include <functional>
//using namespace view3d;
namespace graphics
{

    ModelDrawInspectorFactory::ModelDrawInspectorFactory(const graphics::DevicePointer& device) :
        _sharedUniforms(std::make_shared<ModelDrawInspectorUniforms>()) {
        allocateGPUShared(device);
    }
    ModelDrawInspectorFactory::~ModelDrawInspectorFactory() {
    }

    // Custom data uniforms
    struct ModelObjectData {
        uint32_t nodeID{0};
        uint32_t partID{0};
        uint32_t numNodes{ 0 };
        uint32_t numParts{ 0 };

        uint32_t numMaterials{ 0 };
        uint32_t numEdges{ 0 };
        uint32_t drawMode{ 0 };
        int32_t inspectedTriangle{ -1 };
        int32_t numInspectedTriangles{ 1 };

        float _uvCenterX{ 0.0f };
        float _uvCenterY{ 0.0f };
        float _uvScale{ 1.0f };
        float _colorMapBlend{ 0.5f };

        float _kernelRadius{ 5.0f };

        int32_t _inspectedTexelX{ 0 };
        int32_t _inspectedTexelY{ 0 };
    };

    ModelObjectData makeModelObjectData(const ModelDrawInspectorUniforms& params, int32_t node, uint32_t flags) {
        ModelObjectData odata{
            (uint32_t) node, (uint32_t)0,
            (uint32_t) params.numNodes, (uint32_t) params.numParts,
            (uint32_t) params.numMaterials, (uint32_t) params.numEdges,
            flags,
            (int32_t)params.inspectedTriangle, (int32_t) params.numInspectedTriangles,
            params.uvSpaceCenterX, params.uvSpaceCenterY,
            params.uvSpaceScale, params.colorMapBlend,
            params.kernelRadius,
            (int32_t)params.inspectedTexelX, (int32_t) params.inspectedTexelY
        };
        return (odata);
    }


    void ModelDrawInspectorFactory::allocateGPUShared(const graphics::DevicePointer& device) {

        // Let's describe the draw pipeline Descriptors layout
        graphics::RootDescriptorLayoutInit draw_descriptorLayoutInit{
            {
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::ALL_GRAPHICS, 1, sizeof(ModelObjectData) >> 2},
            },
            {
                // ViewPass descriptorSet Layout
                Viewport::viewPassLayout,
                {
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::ALL_GRAPHICS, 1, 1}, // Part
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 2, 1}, // Index
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 3, 1}, // Vertex
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 4, 1}, // Attrib
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::ALL_GRAPHICS, 5, 1}, // Edge
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::ALL_GRAPHICS, 6, 1}, // Face

                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::PIXEL, 9, 1},  // Material
                { graphics::DescriptorType::RESOURCE_TEXTURE, graphics::ShaderStage::PIXEL, 10, 1},  // Albedo Texture
                { graphics::DescriptorType::RESOURCE_TEXTURE, graphics::ShaderStage::ALL_GRAPHICS, 11, 1},  // UVMesh Texture
                { graphics::DescriptorType::RESOURCE_TEXTURE, graphics::ShaderStage::PIXEL, 12, 1},  // compute Texture
                }
            },
            {
            { graphics::DescriptorType::SAMPLER, graphics::ShaderStage::ALL_GRAPHICS, 0, 2},
            }
        };
        auto draw_rootDescriptorLayout = device->createRootDescriptorLayout(draw_descriptorLayoutInit);

        // Let's describe the uvmesh pipeline Descriptors layout
         graphics::RootDescriptorLayoutInit uvmesh_descriptorLayoutInit{
            {
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::ALL_GRAPHICS, 1, sizeof(ModelObjectData) >> 2},
            },
            {
                // ViewPass descriptorSet Layout
                Viewport::viewPassLayout,
                {
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::ALL_GRAPHICS, 1, 1}, // Part
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 2, 1}, // Index
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 3, 1}, // Vertex
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 4, 1}, // Attrib
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::ALL_GRAPHICS, 5, 1}, // Edge
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::ALL_GRAPHICS, 6, 1}, // Face
                }
            }
        };
        auto uvmesh_rootDescriptorLayout = device->createRootDescriptorLayout(uvmesh_descriptorLayoutInit);

        // Let's describe the pipeline Descriptors layout for compute pass
        graphics::RootDescriptorLayoutInit compute_descriptorLayoutInit{
            {
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::COMPUTE, 0, sizeof(ModelObjectData) >> 2},
            },
            {{
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::COMPUTE, 1, 1}, // Part
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::COMPUTE, 2, 1}, // Index
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::COMPUTE, 3, 1}, // Vertex
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::COMPUTE, 4, 1}, // Attrib
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::COMPUTE, 5, 1}, // Edge
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::COMPUTE, 6, 1}, // Face

            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::COMPUTE, 9, 1},  // Material
            { graphics::DescriptorType::RESOURCE_TEXTURE, graphics::ShaderStage::COMPUTE, 10, 1},  // Albedo Texture
            { graphics::DescriptorType::RESOURCE_TEXTURE, graphics::ShaderStage::COMPUTE, 11, 1},  // UVTool Texture

            { graphics::DescriptorType::RW_RESOURCE_TEXTURE, graphics::ShaderStage::COMPUTE, 0, 1}, // render target!
            }},
            {
            { graphics::DescriptorType::SAMPLER, graphics::ShaderStage::COMPUTE, 0, 2},
            }
        };
        auto compute_rootDescriptorLayout = device->createRootDescriptorLayout(compute_descriptorLayoutInit);

        // Includes for all shaders
        graphics::ShaderIncludeLib include = {
            Transform_inc::getMapEntry(),
            Projection_inc::getMapEntry(),
            Camera_inc::getMapEntry(),
            SceneTransform_inc::getMapEntry(),

            Mesh_inc::getMapEntry(),
            Part_inc::getMapEntry(),
            Material_inc::getMapEntry(),
            SceneModel_inc::getMapEntry(),

            Color_inc::getMapEntry(),
            Paint_inc::getMapEntry(),
            Shading_inc::getMapEntry(),

            Surface_inc::getMapEntry(),
            MeshEdge_inc::getMapEntry(),
            Triangle_inc::getMapEntry(),
        };

        // And a Pipeline

        // Draw pass shaders and pipeline

        graphics::ShaderInit draw_vertexShaderInit{ graphics::ShaderType::VERTEX, "main", ModelInspectorPart_vert::getSource, ModelInspectorPart_vert::getSourceFilename(), include };
        graphics::ShaderPointer draw_vertexShader = device->createShader(draw_vertexShaderInit);

        graphics::ShaderInit draw_pixelShaderInit{ graphics::ShaderType::PIXEL, "main", ModelInspectorPart_frag::getSource, ModelInspectorPart_frag::getSourceFilename(), include };
        graphics::ShaderPointer draw_pixelShader = device->createShader(draw_pixelShaderInit);

        graphics::ProgramInit draw_programInit{ draw_vertexShader, draw_pixelShader };
        graphics::ShaderPointer draw_program = device->createProgram(draw_programInit);

        // Default pipeline draw faces
        graphics::GraphicsPipelineStateInit draw_pipelineInit{
                    draw_program,
                    draw_rootDescriptorLayout,
                    StreamLayout(),
                    graphics::PrimitiveTopology::TRIANGLE,
                    RasterizerState().withCullBack(),
                    true, // enable depth
                    BlendState()
        };
        _pipeline_draw_mesh = device->createGraphicsPipelineState(draw_pipelineInit);

        // Draw mesh seam edges
        graphics::GraphicsPipelineStateInit draw_edges_pipelineInit{
                    draw_program,
                    draw_rootDescriptorLayout,
                    StreamLayout(),
                    graphics::PrimitiveTopology::LINE,
                    RasterizerState(),//.withAntialiasedLine().withConservativeRasterizer(),
                    true, // enable depth
                    BlendState()
        };
        _pipeline_draw_edges = device->createGraphicsPipelineState(draw_edges_pipelineInit);

        // Draw mesh connectivity
        graphics::ShaderInit draw_connectivity_vertexShaderInit{ graphics::ShaderType::VERTEX, "main_connectivity", ModelInspectorPart_vert::getSource, ModelInspectorPart_vert::getSourceFilename(), include };
        graphics::ShaderPointer draw_connectivity_vertexShader = device->createShader(draw_connectivity_vertexShaderInit);

        graphics::ShaderInit draw_connectivity_pixelShaderInit{ graphics::ShaderType::PIXEL, "main_connectivity", ModelInspectorPart_frag::getSource, ModelInspectorPart_frag::getSourceFilename(), include };
        graphics::ShaderPointer draw_connectivity_pixelShader = device->createShader(draw_connectivity_pixelShaderInit);

        graphics::ProgramInit draw_connectivity_programInit{ draw_connectivity_vertexShader, draw_connectivity_pixelShader };
        graphics::ShaderPointer draw_connectivity_program = device->createProgram(draw_connectivity_programInit);

        graphics::GraphicsPipelineStateInit draw_connectivity_pipelineInit{
                    draw_connectivity_program,
                    draw_rootDescriptorLayout,
                    StreamLayout(),
                    graphics::PrimitiveTopology::TRIANGLE,
                    RasterizerState().withCullBack(),//.withAntialiasedLine().withConservativeRasterizer(),
                    true, // enable depth
                    BlendState()
        };
        _pipeline_draw_connectivity = device->createGraphicsPipelineState(draw_connectivity_pipelineInit);

        // Draw mesh connectivity
        graphics::ShaderInit draw_kernelSamples_vertexShaderInit{ graphics::ShaderType::VERTEX, "main_kernelSamples", ModelInspectorPart_vert::getSource, ModelInspectorPart_vert::getSourceFilename(), include };
        graphics::ShaderPointer draw_kernelSamples_vertexShader = device->createShader(draw_kernelSamples_vertexShaderInit);

        graphics::ShaderInit draw_kernelSamples_pixelShaderInit{ graphics::ShaderType::PIXEL, "main_kernelSamples", ModelInspectorPart_frag::getSource, ModelInspectorPart_frag::getSourceFilename(), include };
        graphics::ShaderPointer draw_kernelSamples_pixelShader = device->createShader(draw_kernelSamples_pixelShaderInit);

        graphics::ProgramInit draw_kernelSamples_programInit{ draw_kernelSamples_vertexShader, draw_kernelSamples_pixelShader };
        graphics::ShaderPointer draw_kernelSamples_program = device->createProgram(draw_kernelSamples_programInit);

        graphics::GraphicsPipelineStateInit draw_kernelSamples_pipelineInit{
                    draw_kernelSamples_program,
                    draw_rootDescriptorLayout,
                    StreamLayout(),
                    graphics::PrimitiveTopology::TRIANGLE,
                    RasterizerState().withCullBack(),//.withAntialiasedLine().withConservativeRasterizer(),
                    true, // enable depth
                    BlendState()
        };
        _pipeline_draw_kernelSamples = device->createGraphicsPipelineState(draw_kernelSamples_pipelineInit);


        // Draw some widgets to debug and uderstand the UV space
        graphics::ShaderInit uvspace_vertexShaderInit{ graphics::ShaderType::VERTEX, "main_uvspace", ModelInspectorPart_vert::getSource, ModelInspectorPart_vert::getSourceFilename(), include };
        graphics::ShaderPointer uvspace_vertexShader = device->createShader(uvspace_vertexShaderInit);
        graphics::ShaderInit uvspace_pixelShaderInit{ graphics::ShaderType::PIXEL, "main_uvspace", ModelInspectorPart_frag::getSource, ModelInspectorPart_frag::getSourceFilename(), include };
        graphics::ShaderPointer uvspace_pixelShader = device->createShader(uvspace_pixelShaderInit);

        graphics::ProgramInit uvspace_programInit{ uvspace_vertexShader, uvspace_pixelShader };
        graphics::ShaderPointer uvspace_program = device->createProgram(uvspace_programInit);

        graphics::GraphicsPipelineStateInit uvspace_pipelineInit{
            uvspace_program,
            draw_rootDescriptorLayout,
            StreamLayout(),
            graphics::PrimitiveTopology::TRIANGLE_STRIP,
            RasterizerState(),
            false,
            BlendState()
        }; 
        _pipeline_draw_uvspace_inspect = device->createGraphicsPipelineState(uvspace_pipelineInit);

        // Draw uvmesh map points
        graphics::ShaderInit draw_uvmesh_point_vertexShaderInit{ graphics::ShaderType::VERTEX, "main_uvmesh_point", ModelInspectorPart_vert::getSource, ModelInspectorPart_vert::getSourceFilename(), include };
        graphics::ShaderPointer draw_uvmesh_point_vertexShader = device->createShader(draw_uvmesh_point_vertexShaderInit);

        graphics::ShaderInit draw_uvmesh_point_pixelShaderInit{ graphics::ShaderType::PIXEL, "main_uvmesh_point", ModelInspectorPart_frag::getSource, ModelInspectorPart_frag::getSourceFilename(), include };
        graphics::ShaderPointer draw_uvmesh_point_pixelShader = device->createShader(draw_uvmesh_point_pixelShaderInit);

        graphics::ProgramInit draw_uvmesh_point_programInit{ draw_uvmesh_point_vertexShader, draw_uvmesh_point_pixelShader };
        graphics::ShaderPointer draw_uvmesh_point_program = device->createProgram(draw_uvmesh_point_programInit);

        graphics::GraphicsPipelineStateInit pipeline_draw_uvmesh_point{
            draw_uvmesh_point_program,
            draw_rootDescriptorLayout,
            StreamLayout(),
            graphics::PrimitiveTopology::POINT,
            RasterizerState(),
            true, // enable depth
            BlendState()
        };
        _pipeline_draw_uvmesh_point = device->createGraphicsPipelineState(pipeline_draw_uvmesh_point);


        // 
        // Make uvmesh pipeline draw edges and face and encode mesh data
        graphics::ShaderInit make_uvmesh_pixelShaderInit{ graphics::ShaderType::PIXEL, "main_uvmesh", ModelInspectorPart_frag::getSource, ModelInspectorPart_frag::getSourceFilename(), include };
        graphics::ShaderPointer make_uvmesh_pixelShader = device->createShader(make_uvmesh_pixelShaderInit);

        graphics::ProgramInit make_uvmesh_programInit{ draw_vertexShader, make_uvmesh_pixelShader };
        graphics::ShaderPointer make_uvmesh_program = device->createProgram(make_uvmesh_programInit);

        graphics::GraphicsPipelineStateInit make_uvmesh_pipelineInit{
                    make_uvmesh_program,
                    uvmesh_rootDescriptorLayout,
                    StreamLayout(),
                    graphics::PrimitiveTopology::TRIANGLE,
                    RasterizerState().withConservativeRasterizer().withMultisample(),
                    false,
                    BlendState(),
                    _uvmeshMapFormat // same format as the uvmesh_map
        };
        _pipeline_uvmesh_makeFace = device->createGraphicsPipelineState(make_uvmesh_pipelineInit);
        make_uvmesh_pipelineInit.primitiveTopology = graphics::PrimitiveTopology::LINE;
        make_uvmesh_pipelineInit.rasterizer = RasterizerState().withAntialiasedLine().withMultisample();
        _pipeline_uvmesh_makeEdge = device->createGraphicsPipelineState(make_uvmesh_pipelineInit);

        // First compute blur shader and pipeline
        {
         //   graphics::ShaderInit imageSpaceBlur_compShaderInit{ graphics::ShaderType::COMPUTE, "main_imageSpaceBlur", ModelUVSpaceProcessing_comp::getSource, ModelUVSpaceProcessing_comp::getSourceFilename(), include };
            graphics::ShaderInit imageSpaceBlur_compShaderInit{ graphics::ShaderType::COMPUTE, "main_imageSpaceBlurBrutForce", ModelUVSpaceProcessing_comp::getSource, ModelUVSpaceProcessing_comp::getSourceFilename(), include };
            graphics::ShaderPointer imageSpaceBlur_compShader = device->createShader(imageSpaceBlur_compShaderInit);

            // Let's describe the Compute pipeline Descriptors layout
            graphics::ComputePipelineStateInit imageSpaceBlur_compPipelineInit{
                imageSpaceBlur_compShader,
                compute_rootDescriptorLayout
            };

            _pipeline_compute_imageSpaceBlur = device->createComputePipelineState(imageSpaceBlur_compPipelineInit);
        }
        {
            graphics::ShaderInit meshSpaceBlur_compShaderInit{ graphics::ShaderType::COMPUTE, "main_meshSpaceBlur", ModelUVSpaceProcessing_comp::getSource, ModelUVSpaceProcessing_comp::getSourceFilename(), include };
            graphics::ShaderPointer meshSpaceBlur_compShader = device->createShader(meshSpaceBlur_compShaderInit);

            _pipeline_compute_meshSpaceBlur = device->createComputePipelineState({
                meshSpaceBlur_compShader,
                compute_rootDescriptorLayout
            });
        }
    }




    graphics::ModelDrawInspector* ModelDrawInspectorFactory::createModel(const graphics::DevicePointer& device, const document::ModelPointer& model, const ModelDraw* srcDraw) {

        auto modelDraw = new graphics::ModelDrawInspector();

        modelDraw->_inspectedModelDraw = srcDraw;

        // Define the local nodes used by the model with the original transforms and the parents
        modelDraw->_localNodeTransforms = srcDraw->_localNodeTransforms;
        modelDraw->_localNodeParents = srcDraw->_localNodeParents;
        // Define the items
        modelDraw->_localItems = srcDraw->_localItems;
        // Define the shapes
        modelDraw->_shapes = srcDraw->_shapes;
        // Define the cameras
        modelDraw->_localCameras = srcDraw->_localCameras;


        modelDraw->_uniforms = _sharedUniforms;
        modelDraw->_indexBuffer = srcDraw->_indexBuffer;
        modelDraw->_vertexBuffer = srcDraw->_vertexBuffer;
        modelDraw->_vertexAttribBuffer = srcDraw->_vertexAttribBuffer;
        modelDraw->_partBuffer = srcDraw->_partBuffer;
        modelDraw->_shapes = srcDraw->_shapes;

        // Also need a version of the parts and their bound on the cpu side
        modelDraw->_vertices = srcDraw->_vertices;
        modelDraw->_vertex_attribs = srcDraw->_vertex_attribs;
        modelDraw->_indices = srcDraw->_indices;
        modelDraw->_parts = srcDraw->_parts;
        modelDraw->_partAABBs = srcDraw->_partAABBs;

        modelDraw->_edgeBuffer = srcDraw->_edgeBuffer;
        modelDraw->_faceBuffer = srcDraw->_faceBuffer;

        // material buffer
        modelDraw->_materialBuffer = srcDraw->_materialBuffer;
        modelDraw->_albedoTexture = srcDraw->_albedoTexture;
        
        // Model local bound is the containing box for all the local items of the model
        modelDraw->_bound = srcDraw->_bound;

        // Making the "uvmesh map" which will be rt once with the mesh information and then used to fetch from
        TextureInit uvmeshMapInit;
        uvmeshMapInit.width = modelDraw->_albedoTexture->_init.width;
        uvmeshMapInit.height = modelDraw->_albedoTexture->_init.height;
        uvmeshMapInit.usage = ResourceUsage::RENDER_TARGET;
        uvmeshMapInit.format = _uvmeshMapFormat;
        auto uvmeshMap = device->createTexture(uvmeshMapInit);
        modelDraw->_texture_uvmesh = uvmeshMap;

        // And the framebuffer to render into it
        FramebufferInit uvmeshFramebufferInit;
        uvmeshFramebufferInit.colorTargets.push_back(uvmeshMap);
        auto uvmeshFramebuffer = device->createFramebuffer(uvmeshFramebufferInit);
        modelDraw->_framebuffer_uvmesh = uvmeshFramebuffer;

        // Making one "compute map" which will be rw destination from the compute kernel and then used to fetch from
        TextureInit computeMapInit;
        computeMapInit.width = modelDraw->_albedoTexture->_init.width;
        computeMapInit.height = modelDraw->_albedoTexture->_init.height;
        computeMapInit.usage = ResourceUsage::RW_RESOURCE_TEXTURE;
        auto computeMap = device->createTexture(computeMapInit);
        modelDraw->_texture_compute = computeMap;

        // fill the constant model dimensions in uniforms:
        modelDraw->getUniforms()->numNodes = modelDraw->_localNodeTransforms.size();
        modelDraw->getUniforms()->numParts = modelDraw->getPartBuffer()->numElements();
        modelDraw->getUniforms()->numNodes = modelDraw->getMaterialBuffer()->numElements();
        modelDraw->getUniforms()->numEdges = modelDraw->getEdgeBuffer()->numElements();
        modelDraw->getUniforms()->numTriangles = modelDraw->getFaceBuffer()->numElements();

        modelDraw->getUniforms()->mapWidth = computeMapInit.width;
        modelDraw->getUniforms()->mapHeight = computeMapInit.height;
        modelDraw->getUniforms()->inspectedTexelX = computeMapInit.width >> 1;
        modelDraw->getUniforms()->inspectedTexelY = computeMapInit.height >> 1;


        return modelDraw;
    }

    void ModelDrawInspectorFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        const graphics::ScenePointer& scene,
        graphics::ModelDrawInspector& model)
    {
        {
            // It s time to create a descriptorSet that matches the expected pipeline descriptor set
            // then we will assign a uniform buffer in it
            graphics::DescriptorSetInit descriptorSetInit{
                _pipeline_draw_mesh->getRootDescriptorLayout(),
                1, true
            };
            auto descriptorSet = device->createDescriptorSet(descriptorSetInit);
            model._descriptorSet = descriptorSet;

            // Assign the Camera UBO just created as the resource of the descriptorSet
            graphics::SamplerInit samplerInit{};
            auto sampler = device->createSampler(samplerInit);

            samplerInit._filter = graphics::Filter::MIN_MAG_LINEAR_MIP_POINT;
            auto samplerL = device->createSampler(samplerInit);


            graphics::DescriptorObjects descriptorObjects {
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getPartBuffer() },
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getIndexBuffer() },
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getVertexBuffer() },
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getVertexAttribBuffer() },
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getEdgeBuffer() },
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getFaceBuffer() },
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getMaterialBuffer() },
                { graphics::DescriptorType::RESOURCE_TEXTURE, model.getAlbedoTexture() },
                { graphics::DescriptorType::RESOURCE_TEXTURE, model._texture_uvmesh },
                { graphics::DescriptorType::RESOURCE_TEXTURE, model._texture_compute },
                { sampler },
                { samplerL }
            };
            device->updateDescriptorSet(descriptorSet, descriptorObjects);
        }

        {
            // It s time to create a descriptorSet that matches the expected pipeline descriptor set
            // then we will assign a uniform buffer in it
            graphics::DescriptorSetInit descriptorSetInit{
                _pipeline_uvmesh_makeEdge->getRootDescriptorLayout(),
                1
            };
            auto descriptorSet = device->createDescriptorSet(descriptorSetInit);
            model._descriptorSet_uvmesh = descriptorSet;

            // Assign the Camera UBO just created as the resource of the descriptorSet
            graphics::DescriptorObjects descriptorObjects = {
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getPartBuffer() },
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getIndexBuffer() },
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getVertexBuffer() },
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getVertexAttribBuffer() },
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getEdgeBuffer() },
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getFaceBuffer() }
            };
            device->updateDescriptorSet(descriptorSet, descriptorObjects);
        }

        {
            // It s time to create a descriptorSet that matches the expected pipeline descriptor set
            // then we will assign a uniform buffer in it
            graphics::DescriptorSetInit descriptorSetInit{
                _pipeline_compute_imageSpaceBlur->getRootDescriptorLayout(),
                0, true
            };
            auto descriptorSet = device->createDescriptorSet(descriptorSetInit);
            model._descriptorSet_compute = descriptorSet;

            // Assign the Camera UBO just created as the resource of the descriptorSet
            graphics::SamplerInit samplerInit{};
            auto sampler = device->createSampler(samplerInit);

            samplerInit._filter = graphics::Filter::MIN_MAG_LINEAR_MIP_POINT;
            auto samplerL = device->createSampler(samplerInit);


            graphics::DescriptorObjects descriptorObjects = {
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getPartBuffer() },
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getIndexBuffer() },
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getVertexBuffer() },
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getVertexAttribBuffer() },
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getEdgeBuffer() },
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getFaceBuffer() },
                { graphics::DescriptorType::RESOURCE_BUFFER, model.getMaterialBuffer() },
                { graphics::DescriptorType::RESOURCE_TEXTURE, model.getAlbedoTexture() },
                { graphics::DescriptorType::RESOURCE_TEXTURE, model._texture_uvmesh },
                { graphics::DescriptorType::RW_RESOURCE_TEXTURE, model._texture_compute },
                { sampler },
                { samplerL }
            };
            device->updateDescriptorSet(descriptorSet, descriptorObjects);
        }

        auto numVertices = model.getVertexBuffer()->numElements();
        auto numIndices = model.getIndexBuffer()->numElements();
        auto vertexStride = model.getVertexBuffer()->_init.structStride;
        auto numParts = model.getPartBuffer()->numElements();
        auto numMaterials = model.getMaterialBuffer()->numElements();

        auto numEdges = model.getEdgeBuffer()->numElements();

        // NUmber of nodes in the model
        auto numNodes = model._localNodeTransforms.size();

        auto albedoTex = model.getAlbedoTexture();
        auto pmodel = &model;
        auto descriptorSet_draw = model._descriptorSet;
        auto descriptorSet_uvmesh = model._descriptorSet_uvmesh;
        auto descriptorSet_compute = model._descriptorSet_compute;
        auto uvmeshMap = model._texture_uvmesh;
        auto uvmeshFramebuffer = model._framebuffer_uvmesh;
        auto computeMap = model._texture_compute;

        auto pipeline_draw_mesh = _pipeline_draw_mesh;
        auto pipeline_draw_uvspace = _pipeline_draw_uvspace_inspect;
        auto pipeline_draw_edges = _pipeline_draw_edges;
        auto pipeline_draw_connectivity = _pipeline_draw_connectivity;
        auto pipeline_draw_kernelSamples = _pipeline_draw_kernelSamples;
        auto pipeline_draw_uvmesh_point = _pipeline_draw_uvmesh_point;
        auto pipeline_uvmesh_edge = _pipeline_uvmesh_makeEdge;
        auto pipeline_uvmesh_face = _pipeline_uvmesh_makeFace;
        auto pipeline_compute_imageSpaceBlur = _pipeline_compute_imageSpaceBlur;
        auto pipeline_compute_meshSpaceBlur = _pipeline_compute_meshSpaceBlur;

        // Pre pass draw executed before drawing the model parts:
        // Generate the uvmesh map pass
        // dispatch compute pass(es)
        // draw uvspace background
        {
            auto pre_pass = new ModelDrawInspectorPart();
            pre_pass->_bound = model._bound;
            graphics::DrawObjectCallback drawCallback = [pmodel, albedoTex, uvmeshFramebuffer, uvmeshMap, computeMap,
                descriptorSet_uvmesh, pipeline_uvmesh_edge, pipeline_uvmesh_face,
                descriptorSet_compute, pipeline_compute_imageSpaceBlur, pipeline_compute_meshSpaceBlur,
                descriptorSet_draw, pipeline_draw_uvspace, pipeline_draw_connectivity, pipeline_draw_kernelSamples,
                numEdges, numNodes, numParts, numMaterials](
                    const NodeID node, RenderArgs& args) {
                        static bool first{ true };
                        auto params = pmodel->getUniforms().get();

                        if (params->makeUVMeshMap) {

                            args.batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::SHADER_RESOURCE, graphics::ResourceState::RENDER_TARGET, uvmeshMap);
                            args.batch->bindFramebuffer(uvmeshFramebuffer);

                            args.batch->clear(uvmeshFramebuffer, { 0, 0, 0, 0 });

                            core::vec4 viewport(0, 0, uvmeshFramebuffer->width(), uvmeshFramebuffer->height());
                            args.batch->pushViewport(viewport);
                            args.batch->pushScissor(viewport);

                            // Draw faces of the mesh in the edge map
                            args.batch->bindPipeline(pipeline_uvmesh_face);

                            // descriptor is bound for both passes since it s the same layout signature
                            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
                            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet_uvmesh);
                            
                            ModelObjectData odata = makeModelObjectData(*params, node, ModelDrawInspectorUniforms::MAKE_UVMESH_MAP_BIT);

                            for (int d = 0; d < pmodel->_parts.size(); ++d) {
                                odata.partID = d;
                                args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(ModelObjectData), (const uint8_t*)&odata);
 
                                auto partNumIndices = pmodel->_parts[d].numIndices;
                                args.batch->draw(partNumIndices, 0);
                            }

                            // Draw the edges over
                            if (params->uvmeshEdgeLinesPass) {
                                args.batch->bindPipeline(pipeline_uvmesh_edge);
                         
                                odata.drawMode =    ModelDrawInspectorUniforms::MAKE_UVMESH_MAP_BIT |
                                                    ModelDrawInspectorUniforms::RENDER_UV_EDGE_LINES_BIT;

                                for (int d = 0; d < pmodel->_parts.size(); ++d) {
                                    odata.partID = d;
                                    args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(ModelObjectData), (const uint8_t*)&odata);
                                    auto partNumEdges = pmodel->_parts[d].numEdges;
                                    args.batch->draw(partNumEdges * 2, 0);
                                }
                            }

                            args.batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::RENDER_TARGET, graphics::ResourceState::SHADER_RESOURCE, uvmeshMap);

                            args.batch->beginPass(args.swapchain, args.swapchain->currentIndex());


                            params->makeUVMeshMap = false;
                            args.batch->popViewport();
                            args.batch->popScissor();
                        }

                        if (first) {
                            if (albedoTex) {
                                args.batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::SHADER_RESOURCE, graphics::ResourceState::COPY_DEST, albedoTex);
                                args.batch->uploadTexture(albedoTex);
                                args.batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::COPY_DEST, graphics::ResourceState::SHADER_RESOURCE, albedoTex);
                            }
                        }

                        if (params->makeComputedMap) {
                            const int NUM_COMPUTE_GROUP_THREADS = 4;
                            switch (params->filterKernelTechnique) {
                                case ModelDrawInspectorUniforms::FKT_IMAGE_SPACE:
                                    args.batch->bindPipeline(pipeline_compute_imageSpaceBlur);
                                break;
                                case ModelDrawInspectorUniforms::FKT_MESH_SPACE:
                                    args.batch->bindPipeline(pipeline_compute_meshSpaceBlur);
                                break;
                            }
                            
                            args.batch->bindDescriptorSet(graphics::PipelineType::COMPUTE, descriptorSet_compute);

                            ModelObjectData odata = makeModelObjectData(*params, node, params->buildFlags());
                            args.batch->bindPushUniform(graphics::PipelineType::COMPUTE, 0, sizeof(ModelObjectData), (const uint8_t*)&odata);

                            args.batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::SHADER_RESOURCE, graphics::ResourceState::UNORDERED_ACCESS, computeMap);
                            args.batch->dispatch(computeMap->width() / NUM_COMPUTE_GROUP_THREADS, computeMap->height() / NUM_COMPUTE_GROUP_THREADS);
                            args.batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::UNORDERED_ACCESS, graphics::ResourceState::SHADER_RESOURCE, computeMap);

                            params->makeComputedMap = false;
                        }

                        // in uv space mode, draw uvspace inspect quad
                        if (params->renderUVSpace) {
                            args.batch->bindPipeline(pipeline_draw_uvspace);

                            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
                            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet_draw);

                            ModelObjectData odata = makeModelObjectData(*params, node, params->buildFlags());
                            args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(ModelObjectData), (const uint8_t*)&odata);

                            args.batch->draw(4, 0); // draw quad
                        }


                        if (params->renderConnectivity && (params->inspectedTriangle > -1)) {
                            args.batch->bindPipeline(pipeline_draw_connectivity);

                            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
                            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet_draw);

                            auto flags = params->buildFlags();
                            ModelObjectData odata = makeModelObjectData(*params, node, flags);

                            for (int d = 0; d < pmodel->_parts.size(); ++d) {
                                odata.partID = d;
                                args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(ModelObjectData), (const uint8_t*)&odata);
                                auto partNumIndices = pmodel->_parts[d].numIndices;
                                args.batch->draw((params->numInspectedTriangles) * 3, 0);
                            }
                        }

                        if (params->renderKernelSamples && (params->inspectedTexelX > -1) && (params->inspectedTexelY > -1)) {
                            args.batch->bindPipeline(pipeline_draw_kernelSamples);

                            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
                            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet_draw);

                            ModelObjectData odata = makeModelObjectData(*params, node, params->buildFlags());
                            args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(ModelObjectData), (const uint8_t*)&odata);

                            args.batch->draw((params->numKernelSamples + 1) * 3, 0); // draw num samples per primitive
                        }


                        first = false;
            };

            pre_pass->_drawcall = drawCallback;
            model._pre_pass_ID = scene->createDraw(*pre_pass).id();

        }

        // Standard drawcalls for thee model and one drawcall per part
        {
            {
                graphics::DrawObjectCallback drawCallback = [descriptorSet_draw, pipeline_draw_mesh](
                    const NodeID node, RenderArgs& args) {


                    // this all works because the main texture is populated in the very first drawcall on the first call

                    args.batch->bindPipeline(pipeline_draw_mesh);

                    args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
                    args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet_draw);
                };
                model._drawcall = drawCallback;
                model._drawID = scene->createDraw(model).id();
            }
            {
                // one draw per part
                DrawIDs drawables;
                for (int d = 0; d < model._partAABBs.size(); ++d) {
                    auto part = new ModelDrawInspectorPart();
                    part->_bound = model._partAABBs[d];

                    auto partNumIndices = model._parts[d].numIndices;
                    // And now a render callback where we describe the rendering sequence
                    graphics::DrawObjectCallback drawCallback = [pmodel, d, partNumIndices](
                        const NodeID node,
                        RenderArgs& args) {
                            auto params = pmodel->getUniforms().get();
                            if (params->render3DModel) {
                                ModelObjectData odata = makeModelObjectData(*params, node, params->buildFlags());
                                odata.partID = d;
                                args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(ModelObjectData), (const uint8_t*)&odata);

                                args.batch->draw(partNumIndices, 0);
                            }
                    };

                    part->_drawcall = drawCallback;

                    auto partDraw = scene->createDraw(*part);
                    drawables.emplace_back(partDraw.id());
                }

               model._partDraws = drawables;
            }
        }

        // one more draw for POST pass to draw extra widgets,  edges or uvmesh points
        {
            auto post_pass = new ModelDrawInspectorPart();
            post_pass->_bound = model._bound;
            // And now a render callback where we describe the rendering sequence
            graphics::DrawObjectCallback drawCallback = [pmodel, uvmeshMap,
                descriptorSet_draw, pipeline_draw_edges, pipeline_draw_uvmesh_point, pipeline_draw_connectivity,
                numEdges, numNodes, numParts, numMaterials](
                const NodeID node, RenderArgs& args) {
                    auto params = pmodel->getUniforms().get();

                    if (params->renderWireframe || params->renderUVEdgeLines) {
                        args.batch->bindPipeline(pipeline_draw_edges);

                        args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
                        args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet_draw);

                        auto flags = params->buildFlags();
                        flags |= ModelDrawInspectorUniforms::RENDER_UV_EDGE_LINES_BIT;
                        
                        if (params->renderWireframe) {
                            flags |= ModelDrawInspectorUniforms::RENDER_WIREFRAME_BIT;
                        }
                        ModelObjectData odata = makeModelObjectData(*params, node, flags);

                        for (int d = 0; d < pmodel->_parts.size(); ++d) {
                            odata.partID = d;
                            args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(ModelObjectData), (const uint8_t*)&odata);
                            auto partNumEdges = pmodel->_parts[d].numEdges;
                            args.batch->draw(partNumEdges * 2, 0);
                        }
                    }


                    // draw the cloud point of samples from the uv mesh
                    if (params->renderUVMeshPoints) {
                        args.batch->bindPipeline(pipeline_draw_uvmesh_point);

                        args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
                        args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet_draw);

                        ModelObjectData odata = makeModelObjectData(*params, node, params->buildFlags());
                        args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(ModelObjectData), (const uint8_t*)&odata);

                        args.batch->draw(uvmeshMap->width() * uvmeshMap->height(), 0); // draw point cloud
                    }

            };

            post_pass->_drawcall = drawCallback;
            model._post_pass_ID = scene->createDraw(*post_pass).id();
        }


    }

    graphics::ItemIDs ModelDrawInspectorFactory::createModelParts(
                    const graphics::NodeID root,
                    const graphics::ScenePointer& scene,
                    graphics::ModelDrawInspector& model) {
   
        auto rootNode = scene->createNode(core::mat4x3(), root);


        // Allocating the new instances of scene::nodes, one per local node
        auto modelNodes = scene->createNodeBranch(rootNode.id(), model._localNodeTransforms, model._localNodeParents);

        // Allocate the new scene::items combining the localItem's node with every shape parts
        graphics::ItemIDs items;

        // Pre pass item 
        items.emplace_back(scene->createItem(rootNode.id(), model._pre_pass_ID).id());

        // first item is the model draw
        items.emplace_back(scene->createItem(rootNode.id(), model._drawID).id());

        // one item per parts
        for (const auto& li : model._localItems) {
            if (li.shape != MODEL_INVALID_INDEX) {
                const auto& s = model._shapes[li.shape];
                for (uint32_t si = 0; si < s.numParts; ++si) {
                    items.emplace_back(scene->createItem(modelNodes[li.node], model._partDraws[si + s.partOffset]).id());
                }
            }
            if (li.camera != MODEL_INVALID_INDEX) {
                
            }
        }

        // One more item for post
        items.emplace_back(scene->createItem(rootNode.id(), model._post_pass_ID).id());

        return items; 
    }


} // !namespace graphics