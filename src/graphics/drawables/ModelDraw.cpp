// ModelDraw.cpp
//
// Sam Gateau - January 2020
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
#include "ModelDraw.h"

#include "core/stl/Hash.h"
#include "gpu/Device.h"
#include "gpu/Batch.h"
#include "gpu/Shader.h"
#include "gpu/Resource.h"
#include "gpu/Pipeline.h"
#include "gpu/Descriptor.h"
#include "gpu/Swapchain.h"

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
#include "Skin_inc.h"
#include "Material_inc.h"
#include "SceneModel_inc.h"

#include "Color_inc.h"
#include "Paint_inc.h"
#include "Shading_inc.h"
#include "Surface_inc.h"
#include "Sky_inc.h"

#include "ModelPart_vert.h"
#include "ModelPart_frag.h"


//using namespace view3d;
namespace graphics
{

    ModelDrawFactory::ModelDrawFactory(const graphics::DevicePointer& device) :
        _sharedUniforms(std::make_shared<ModelDrawUniforms>()) {
        allocateGPUShared(device);
    }
    ModelDrawFactory::~ModelDrawFactory() {

    }

    uint32_t ModelDrawUniforms::makeDrawMode() const {

        return displayedColor | (lightShading ? 0x80 : 0);
    }

    // Custom data uniforms
    struct ModelObjectData {
        uint32_t nodeID{0};
        uint32_t partID{0};
        uint32_t numNodes{ 0 };
        uint32_t numParts{ 0 };
        uint32_t numMaterials{ 0 };
        uint32_t drawMode{ 0 };
    };

    void ModelDrawFactory::allocateGPUShared(const graphics::DevicePointer& device) {

        // Let's describe the pipeline Descriptors layout
        graphics::RootDescriptorLayoutInit descriptorLayoutInit{
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
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 5, 1}, // Skin
                { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::PIXEL, 9, 1 },  // Material
                { graphics::DescriptorType::RESOURCE_TEXTURE, graphics::ShaderStage::PIXEL, 10, 1 },  // Material Textures
                }
            },
            {
            { graphics::DescriptorType::SAMPLER, graphics::ShaderStage::ALL_GRAPHICS, 0, 2},
            }
        };
        auto rootDescriptorLayout = device->createRootDescriptorLayout(descriptorLayoutInit);
      
        // And a Pipeline
        
        // test: create shader
        graphics::ShaderIncludeLib include = {
            Transform_inc::getMapEntry(),
            Projection_inc::getMapEntry(),
            Camera_inc::getMapEntry(),
            SceneTransform_inc::getMapEntry(),

            Mesh_inc::getMapEntry(),
            Part_inc::getMapEntry(),
            Skin_inc::getMapEntry(),
            Material_inc::getMapEntry(),
            SceneModel_inc::getMapEntry(),
            
            Color_inc::getMapEntry(),
            Paint_inc::getMapEntry(),
            Shading_inc::getMapEntry(),
            Surface_inc::getMapEntry(),

            Sky_inc::getMapEntry(),
        };
        graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "main", ModelPart_vert::getSource, ModelPart_vert::getSourceFilename(), include };
        graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main", ModelPart_frag::getSource, ModelPart_frag::getSourceFilename(), include };
        graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

        graphics::ProgramInit programInit{ vertexShader, pixelShader };
        graphics::ShaderPointer programShader = device->createProgram(programInit);

        graphics::GraphicsPipelineStateInit pipelineInit{
                    programShader,
                    rootDescriptorLayout,
                    StreamLayout(),
                    graphics::PrimitiveTopology::TRIANGLE,
                    RasterizerState().withCullBack(),
                    {true}, // enable depth
                    BlendState()
        };
        _pipeline = device->createGraphicsPipelineState(pipelineInit);
    }

    size_t hashV(ModelVertex& v) {
            core::hash<float, float, float, int> hasher;
            return hasher(v.px, v.py, v.pz, v.n);
    }
    size_t hashA(ModelVertexAttrib& a) {
            return  *reinterpret_cast<uint64_t*>((uint64_t*)&a);
    }

    graphics::ModelDraw* ModelDrawFactory::createModel(const graphics::DevicePointer& device, const document::ModelPointer& model) {

        auto modelDraw = new graphics::ModelDraw();

        modelDraw->_name = model->_name;

        if (model->_scenes.size())
            modelDraw->_localRootNodes = model->_scenes[0]._nodes;

        // Define the local nodes used by the model with the original transforms and the parents
        modelDraw->_localNodeTransforms.reserve(model->_nodes.size());
        modelDraw->_localNodeParents.reserve(model->_nodes.size());
        modelDraw->_localNodeNames.reserve(model->_nodes.size());
        for (const auto& n : model->_nodes) {
            modelDraw->_localNodeTransforms.emplace_back(n._transform);
            modelDraw->_localNodeParents.emplace_back(n._parent);
            modelDraw->_localNodeNames.emplace_back(n._name);
        }

        // Define the items
        modelDraw->_localItems.reserve(model->_items.size());
        for (const auto& si : model->_items) {
            modelDraw->_localItems.emplace_back(ModelItem{
                .node = si._node,
                .shape = si._mesh,
                .skin = si._skin,
                .camera = si._camera });
        }

        // Define the shapes
        modelDraw->_shapes.reserve(model->_meshes.size());
        for (const auto& m : model->_meshes) {
            modelDraw->_shapes.emplace_back(ModelShape{ m._primitiveStart, m._primitiveCount });
        }

        // Define the cameras
        modelDraw->_localCameras.reserve(model->_cameras.size());
        for (const auto& cam : model->_cameras) {
            modelDraw->_localCameras.emplace_back(ModelCamera{ cam._projection });
        }

        // Build the geometry vb, ib and pb
        // as long as the vertex buffer is  less than 65535 the indices can be uint16
        std::vector<ModelVertex> vertex_buffer;
        std::vector<ModelVertexAttrib> vertex_attrib_buffer;
        std::vector<ModelIndex> index_buffer;
        std::vector<ModelPart> parts;
        std::vector<core::aabox3> partAABBs;
        core::aabox3 bound;

        std::vector<uint32_t> mainVertexIndices;

        using LookupAttribArray = std::vector<std::pair<size_t, uint32_t>>;
        using LookupVertex = struct { uint32_t index; LookupAttribArray attribs; };
        using LookupVertexIndexTable = std::unordered_map<size_t, LookupVertex>;
        LookupVertexIndexTable indexedVertexMap;

        std::vector<ModelEdge> edge_buffer;
        std::vector<ModelFace> face_buffer;

        struct LookupEdge { uint32_t index; uint32_t t0; uint32_t t1{ (uint32_t) -1 }; };
        using LookupEdgeIndexedTable = std::unordered_map<size_t, LookupEdge >;
        LookupEdgeIndexedTable indexedEdgeMap;


        bool first = true;
        for (const auto& p : model->_primitives) {
            // Index accessor
            std::vector<ModelIndex> partIndices;
            if (p._indices != document::model::INVALID_INDEX) {
                const auto& indexAccess = model->_accessors[p._indices];
                const auto& indexView = model->_bufferViews[indexAccess._bufferView];
                const auto& indexBuffer = model->_buffers[indexView._buffer];

                auto indexStride = (indexView._byteStride ? indexView._byteStride : document::model::componentTypeSize(indexAccess._componentType));
                uint32_t indexMask = document::model::componentTypeInt32Mask(indexAccess._componentType);
                for (uint32_t i = 0; i < indexAccess._elementCount; ++i) {
                    auto index = (*(uint32_t*)(indexBuffer._bytes.data() + indexView._byteOffset + indexAccess._byteOffset + indexStride * i)) & indexMask;
                    partIndices.emplace_back(index);
                }
            }
            else {
                auto indexCount = model->_accessors[p._positions]._elementCount;
                partIndices.reserve(indexCount);
                for (uint32_t i = 0; i < indexCount; ++i) {
                   partIndices.emplace_back(i);
                }
            }

            // Position accessor
            const auto& posAccess = model->_accessors[p._positions];
            const auto& posView = model->_bufferViews[posAccess._bufferView];
            const auto& posBuffer = model->_buffers[posView._buffer];
            auto posStride = (posView._byteStride ? posView._byteStride : document::model::elementTypeComponentCount(posAccess._elementType) * sizeof(float));
            std::function<core::vec3(uint32_t)> positionGetter = [&](uint32_t index) {
                auto pos = (float*)(posBuffer._bytes.data() + posView._byteOffset + posAccess._byteOffset + posStride * index);
                return core::vec3(*pos, *(pos + 1), *(pos + 2));
            };

            // Normal accessor
            std::function<uint32_t(uint32_t)> normalGetter = [](uint32_t index) { return 0; };
            if (p._normals != document::model::INVALID_INDEX) {
                const auto& norAccess = model->_accessors[p._normals];
                const auto& norView = model->_bufferViews[norAccess._bufferView];
                const auto& norBuffer = model->_buffers[norView._buffer];
                auto norStride = (norView._byteStride ? norView._byteStride : document::model::elementTypeComponentCount(norAccess._elementType) * sizeof(float));
                normalGetter = [&](uint32_t index) {
                    auto nor = (float*)(norBuffer._bytes.data() + norView._byteOffset + norAccess._byteOffset + norStride * index);
                    auto normal = core::vec3(*nor, *(nor + 1), *(nor + 2));
                    return core::packNormal32I(normal);
                };
            }

            // Texcoord accessor
            std::function<core::vec2(uint32_t)> texcoordGetter = [](uint32_t index) { return core::vec2(); };
            if (p._texcoords != document::model::INVALID_INDEX) {
                const auto& texcoordAccess = model->_accessors[p._texcoords];
                const auto& texcoordView = model->_bufferViews[texcoordAccess._bufferView];
                const auto& texcoordBuffer = model->_buffers[texcoordView._buffer];
                auto texcoordStride = (texcoordView._byteStride ? texcoordView._byteStride : document::model::elementTypeComponentCount(texcoordAccess._elementType) * sizeof(float));
                texcoordGetter = [&](uint32_t index) {
                    auto texcoord = (float*)(texcoordBuffer._bytes.data() + texcoordView._byteOffset + texcoordAccess._byteOffset + texcoordStride * index);
                    return core::vec2(*texcoord, *(texcoord + 1));
                };
            }

            // Skin weight and joint accessor
            std::function<core::ivec2(uint32_t)> skinWeightJointGetter = [](uint32_t index) { return core::ivec2(); };
            if ((p._weights != document::model::INVALID_INDEX) && (p._joints != document::model::INVALID_INDEX)) {
                const auto& weightAccess = model->_accessors[p._weights];
                const auto& weightView = model->_bufferViews[weightAccess._bufferView];
                const auto& weightBuffer = model->_buffers[weightView._buffer];
                auto weightComponentSize = document::model::componentTypeSize(weightAccess._componentType);
                auto weightElementCount = document::model::elementTypeComponentCount(weightAccess._elementType);
                auto weightStride = (weightView._byteStride ? weightView._byteStride : weightComponentSize * weightElementCount);

                const auto& jointAccess = model->_accessors[p._joints];
                const auto& jointView = model->_bufferViews[jointAccess._bufferView];
                const auto& jointBuffer = model->_buffers[jointView._buffer];
                auto jointComponentSize = document::model::componentTypeSize(jointAccess._componentType);
                auto jointElementCount = document::model::elementTypeComponentCount(jointAccess._elementType);
                auto jointStride = (jointView._byteStride ? jointView._byteStride : jointComponentSize * jointElementCount);
                auto jointMask = document::model::componentTypeInt32Mask(jointAccess._componentType);

                skinWeightJointGetter = [&](uint32_t index) {
                    auto weight_m = (weightBuffer._bytes.data() + weightView._byteOffset + weightAccess._byteOffset + weightStride * index);
                    core::vec4 vw;
                    vw.x = *(float*)(weight_m + 0);
                    vw.y = *(float*)(weight_m + 1 * weightComponentSize);
                    vw.z = *(float*)(weight_m + 2 * weightComponentSize);
                    vw.w = *(float*)(weight_m + 3 * weightComponentSize);

                    auto joint_m = (jointBuffer._bytes.data() + jointView._byteOffset + jointAccess._byteOffset + jointStride * index);
                    core::ivec4 vj;
                    vj.x = jointMask & *((int32_t*)(joint_m + 0));
                    vj.y = jointMask & *((int32_t*)(joint_m + 1 * jointComponentSize));
                    vj.z = jointMask & *((int32_t*)(joint_m + 2 * jointComponentSize));
                    vj.w = jointMask & *((int32_t*)(joint_m + 3 * jointComponentSize));

                    float sumWeight = core::dot(vw, 1.0f);
                    uint32_t viw = 0;
                    uint32_t vij = 0;
                    if (sumWeight > 0.0f) {
                        for (int i = 0; i < 4; ++i) {
                            uint32_t iw = (0xff & (uint32_t)(255 * vw[i]));
                            uint32_t ij = (0xff & (uint32_t)(vj[i]));
                            viw = viw | ((iw) << (i * 8));
                            vij = vij | ((ij) << (i * 8));
                        }
                    }
                    return core::ivec2(viw, vij);
                };
            }

            std::vector<ModelVertex> partVerts;
            std::vector<ModelVertexAttrib> partAttribs;

            for (uint32_t i = 0; i < partIndices.size(); ++i) {
                uint32_t index = partIndices[i];
                auto vp = positionGetter(index);
                auto vn = normalGetter(index);
                auto vt = texcoordGetter(index);

                auto swj = skinWeightJointGetter(index);

                ModelVertex v{ vp.x, vp.y, vp.z, vn };
                partVerts.emplace_back(v);

                ModelVertexAttrib a{ vt.x, vt.y, swj.x, swj.y };
                partAttribs.emplace_back(a);

                //hash a key on vector pos and normal
                size_t kv = hashV(v);
                size_t ka = hashA(a);

                auto v_bucket = indexedVertexMap.find(kv);
                if (v_bucket == indexedVertexMap.end()) {
                    index = vertex_buffer.size();

                    vertex_buffer.emplace_back(v);
                    vertex_attrib_buffer.emplace_back(a);

                    indexedVertexMap.insert({ kv, { index, {{ka, index}} } });

                    mainVertexIndices.emplace_back(index);
                } else {
                    auto& bucket = v_bucket->second.attribs;
                    index = -1;
                    for (auto& va : bucket) {
                        if (va.first == ka) {
                            index = va.second;
                            break;
                        }
                    }
                    if (index == -1) {
                        index = vertex_buffer.size();
                        vertex_buffer.emplace_back(v);
                        vertex_attrib_buffer.emplace_back(a);
                        bucket.emplace_back(std::pair(ka, index));

                        mainVertexIndices.emplace_back(v_bucket->second.index);
                    }
                }

                // update the index buffer index
                partIndices[i] = index;
            }

            // Record edge and face:
            std::vector<uint32_t> partEdges;
            std::vector<ModelFace> partFaces;
            std::unordered_map<size_t, std::vector<uint32_t>> partMainEdges;
            int32_t numTriangles = partIndices.size() / 3;
            partFaces.reserve(numTriangles);

            for (uint32_t ti = 0; ti < numTriangles; ++ti) {
                uint32_t baseFaceVertexIdx = 3 * ti;
                core::ivec3 triangleVertIds(
                    partIndices[baseFaceVertexIdx + 0],
                    partIndices[baseFaceVertexIdx + 1],
                    partIndices[baseFaceVertexIdx + 2]);

                core::ivec2 edgeVertIndices[3] = {
                  {triangleVertIds.x, triangleVertIds.y},
                  {triangleVertIds.y, triangleVertIds.z},
                  {triangleVertIds.z, triangleVertIds.x}
                };

                core::ivec4 faceEdges;
                for (uint32_t vi = 0; vi < 3; ++vi) {
                    uint32_t edgeIdx = baseFaceVertexIdx + vi;
                    uint32_t edge_index = edgeIdx;

                    auto edge = core::sort_increasing(edgeVertIndices[vi]);
                    auto ek = *reinterpret_cast<uint64_t*>(&edge);

                    auto e_bucket = indexedEdgeMap.find(ek);
                    if (e_bucket == indexedEdgeMap.end()) {
                        edge_index = edge_buffer.size();
                        edge_buffer.emplace_back(core::ivec4{ edge.x, edge.y, (int32_t) ti, -1});
                        indexedEdgeMap.insert({ ek, {edge_index, ti} });
                    } else {
                        picoAssert( (((int32_t) e_bucket->second.t1) == (-1))); // "we have a problem, more than 2 triangles for that edge ???"
                        
                        edge_index = e_bucket->second.index;
                        e_bucket->second.t1 = ti;
                        edge_buffer[edge_index].w = ti;
                    }

                    partEdges.emplace_back(edge_index);
                    faceEdges[vi] = edge_index;

                    
                    core::ivec2 mainEdge{ (int32_t) mainVertexIndices[edge.x], (int32_t) mainVertexIndices[edge.y] };
                    mainEdge = core::sort_increasing(mainEdge);
                    auto mek = *reinterpret_cast<uint64_t*>(&mainEdge);
                    auto me_bucket = partMainEdges.find(mek);
                    if (me_bucket == partMainEdges.end()) {
                        partMainEdges.insert({ mek, {edge_index} });
                    } else {
                        me_bucket->second.emplace_back(edge_index);
                    }
                }
                partFaces.emplace_back(faceEdges);
            }


            
            for (uint32_t ei = 0; ei < partEdges.size(); ++ei) {
                auto edgeId = partEdges[ei];
                auto& edge = edge_buffer[edgeId];

                if (edge.w == -1) {
                    core::ivec2 mainEdge{ (int32_t)mainVertexIndices[edge.x], (int32_t)mainVertexIndices[edge.y] };
                    mainEdge = core::sort_increasing(mainEdge);
                    auto mek = *reinterpret_cast<uint64_t*>(&mainEdge);
                    auto me_bucket = partMainEdges.find(mek);
                    if (me_bucket != partMainEdges.end()) {
                        if (me_bucket->second.size() > 1) {
                            if (me_bucket->second.size() == 2) {
                                for (auto lei : me_bucket->second) {
                                    if (lei != edgeId) {
                                        // go get that other edge and find the neighbor
                                        auto t1 = edge_buffer[lei].z;
                                        edge.w = -(1 + t1);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            ModelPart part{
                .numIndices = (uint32_t)partIndices.size(),
                .indexOffset = (uint32_t)index_buffer.size(),
                .vertexOffset = 0,
                .attribOffset =  0,
                .material = p._material,
                .numEdges = (uint32_t)partEdges.size(),
                .edgeOffset = 0,
                .skinOffset = MODEL_INVALID_INDEX };
             parts.emplace_back(part);
            
            // Fill the index_buffer with the true indices
            for (auto i : partIndices) {
                index_buffer.emplace_back(i);
            }
            // Fill the face_edge_buffer with the true indices
            for (auto i : partFaces) {
                face_buffer.emplace_back(i);
            }

            partAABBs.emplace_back(posAccess._aabb);
            if (first) {
                bound = posAccess._aabb;
                first = false;
            }
            else {
                bound = core::aabox3::fromBound(bound, posAccess._aabb);
            }
        }

        // parts
        BufferInit partBufferInit;
        partBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        partBufferInit.bufferSize = parts.size() * sizeof(ModelPart);
        partBufferInit.hostVisible = true; // TODO Change this to immutable and initialized value
        partBufferInit.firstElement = 0;
        partBufferInit.numElements = parts.size();
        partBufferInit.structStride = sizeof(ModelPart);

        auto pbuniformBuffer = device->createBuffer(partBufferInit);
        memcpy(pbuniformBuffer->_cpuMappedAddress, parts.data(), partBufferInit.bufferSize);

        // vertex buffer
        BufferInit vertexBufferInit;
        vertexBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        vertexBufferInit.bufferSize = vertex_buffer.size() * sizeof(core::vec4);
        vertexBufferInit.hostVisible = true; // TODO Change this to immutable and initialized value
        vertexBufferInit.firstElement = 0;
        vertexBufferInit.numElements = vertex_buffer.size();
        vertexBufferInit.structStride = sizeof(core::vec4);

        auto vbresourceBuffer = device->createBuffer(vertexBufferInit);
        memcpy(vbresourceBuffer->_cpuMappedAddress, vertex_buffer.data(), vertexBufferInit.bufferSize);

        // vertex attrib buffer
        BufferInit vertexattribBufferInit;
        BufferPointer vabresourceBuffer;
        if (vertex_attrib_buffer.size()) {
            vertexattribBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
            vertexattribBufferInit.bufferSize = vertex_attrib_buffer.size() * sizeof(core::vec4);
            vertexattribBufferInit.hostVisible = true; // TODO Change this to immutable and initialized value
            vertexattribBufferInit.firstElement = 0;
            vertexattribBufferInit.numElements = vertex_attrib_buffer.size();
            vertexattribBufferInit.structStride = sizeof(core::vec4);

            vabresourceBuffer = device->createBuffer(vertexattribBufferInit);
            memcpy(vabresourceBuffer->_cpuMappedAddress, vertex_attrib_buffer.data(), vertexattribBufferInit.bufferSize);
        }

        // index buffer
        BufferInit indexBufferInit;
        indexBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        indexBufferInit.bufferSize = index_buffer.size() * sizeof(ModelIndex);
        indexBufferInit.hostVisible = true; // TODO Change this to immutable and initialized value
        indexBufferInit.firstElement = 0;
        indexBufferInit.numElements = index_buffer.size();
        indexBufferInit.structStride = sizeof(ModelIndex);

        auto ibresourceBuffer = device->createBuffer(indexBufferInit);
        memcpy(ibresourceBuffer->_cpuMappedAddress, index_buffer.data(), indexBufferInit.bufferSize);


        // edge buffer
        BufferInit edgeBufferInit;
        edgeBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        edgeBufferInit.bufferSize = edge_buffer.size() * sizeof(ModelEdge);
        edgeBufferInit.hostVisible = true; // TODO Change this to immutable and initialized value
        edgeBufferInit.firstElement = 0;
        edgeBufferInit.numElements = edge_buffer.size();
        edgeBufferInit.structStride = sizeof(ModelEdge);

        auto ebuniformBuffer = device->createBuffer(edgeBufferInit);
        memcpy(ebuniformBuffer->_cpuMappedAddress, edge_buffer.data(), edgeBufferInit.bufferSize);

        modelDraw->_edges = std::move(edge_buffer);
        modelDraw->_edgeBuffer = ebuniformBuffer;

        // Face buffer
        BufferInit faceBufferInit;
        faceBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        faceBufferInit.bufferSize = face_buffer.size() * sizeof(ModelFace);
        faceBufferInit.hostVisible = true; // TODO Change this to immutable and initialized value
        faceBufferInit.firstElement = 0;
        faceBufferInit.numElements = face_buffer.size();
        faceBufferInit.structStride = sizeof(ModelFace);

        auto fbuniformBuffer = device->createBuffer(faceBufferInit);
        memcpy(fbuniformBuffer->_cpuMappedAddress, face_buffer.data(), faceBufferInit.bufferSize);

        modelDraw->_faces = std::move(face_buffer);
        modelDraw->_faceBuffer = fbuniformBuffer;


        modelDraw->_uniforms = _sharedUniforms;
        modelDraw->_indexBuffer = ibresourceBuffer;
        modelDraw->_vertexBuffer = vbresourceBuffer;
        modelDraw->_vertexAttribBuffer = vabresourceBuffer;
        modelDraw->_partBuffer = pbuniformBuffer;

        // Also need a version of the mesh and parts and their bound on the cpu side
        modelDraw->_vertices = std::move(vertex_buffer);
        modelDraw->_vertex_attribs = std::move(vertex_attrib_buffer);
        modelDraw->_indices = std::move(index_buffer);
        modelDraw->_parts = std::move(parts);
        modelDraw->_partAABBs = std::move(partAABBs);



        //// Ray tracing data structure
        //D3D12_RAYTRACING_GEOMETRY_DESC geometry;
        //geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        //geometry.Triangles.VertexBuffer.StartAddress = vb->GetGPUVirtualAddress();
        //geometry.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
        //geometry.Triangles.VertexCount = static_cast<UINT>(vertices.size());
        //geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        //geometry.Triangles.IndexBuffer = ib->GetGPUVirtualAddress();
        //geometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
        //geometry.Triangles.IndexCount = static_cast<UINT>(indices.size());
        //geometry.Triangles.Transform3x4 = 0;
        //geometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

        GeometryInit geometryInit = {
            { vbresourceBuffer, 0, sizeof(core::vec4)},
            (uint32_t) modelDraw->_vertices.size(),
            PixelFormat::R32G32B32_FLOAT,
            { ibresourceBuffer, 0 , 4},
            (uint32_t) modelDraw->_indices.size()
        };
        auto geometry = device->createGeometry(geometryInit);

        
        modelDraw->_geometry = geometry;



        // Materials
        std::vector<ModelMaterial> materials;
        for (const auto& m : model->_materials) {
            ModelMaterial mm;
            mm.color = m._baseColor;
            mm.metallic = m._metallicFactor;
            mm.roughness = m._roughnessFactor;
            mm.baseColorTexture = m._baseColorTexture;
            mm.normalTexture = m._normalTexture;
            mm.rmaoTexture = m._roughnessMetallicTexture;
            mm.emissiveTexture = m._emissiveTexture;
            materials.emplace_back(mm);
        }

        // material buffer
        BufferInit materialBufferInit;
        materialBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        materialBufferInit.bufferSize = materials.size() * sizeof(ModelMaterial);
        materialBufferInit.hostVisible = true; // TODO Change this to immutable and initialized value
        materialBufferInit.firstElement = 0;
        materialBufferInit.numElements = materials.size();
        materialBufferInit.structStride = sizeof(ModelMaterial);

        auto mbresourceBuffer = device->createBuffer(materialBufferInit);
        memcpy(mbresourceBuffer->_cpuMappedAddress, materials.data(), materialBufferInit.bufferSize);

        modelDraw->_materialBuffer = mbresourceBuffer;

        // Allocate the textures
        if (model->_images.size()) {
            uint32_t numImages = model->_images.size();
            uint32_t maxWidth = 0;
            uint32_t maxHeight = 0;
            std::vector<std::vector<uint8_t>> pixels;
            for (auto& i : model->_images) {
                maxWidth = core::max(maxWidth, i._desc.width);
                maxHeight = core::max(maxHeight, i._desc.height);
                pixels.emplace_back(std::move(i._pixels));
            }
            
            if (maxWidth > 0 && maxHeight > 0 && numImages > 0) {
                TextureInit texInit;
                texInit.width = maxWidth;
                texInit.height = maxHeight;
                texInit.numSlices = numImages;
                texInit.initData = std::move(pixels);
                texInit.format = graphics::PixelFormat::R8G8B8A8_UNORM_SRGB;
                auto albedoresourceTexture = device->createTexture(texInit);

                modelDraw->_albedoTexture = albedoresourceTexture;
            }
        }

        // Skin buffer
        {
            for (const auto& s : model->_skins) {
                ModelSkin skin = { .jointOffset = (uint32_t) modelDraw->_skinJointBindings.size(),
                                    .numJoints = (uint32_t) s._joints.size() };
                modelDraw->_skins.emplace_back(skin);

                const auto& accessor = model->_accessors[s._inverseBindMatrices];
                const auto& bufferView = model->_bufferViews[accessor._bufferView];
                const auto& buffer = model->_buffers[bufferView._buffer];

                struct JointMat {
                    float m[16];
                };
                for (int i = 0; i < s._joints.size(); ++i) {
                    int32_t jointNodeId = s._joints[i];
                    JointMat m = document::model::FetchBuffer<JointMat>(i, accessor, bufferView, buffer);
//                    core::mat4x4 inBindMat = document::model::FetchBuffer<core::mat4x4>(i, accessor, bufferView, buffer);
                    ModelSkinJointBinding binding;
                    for (int c = 0; c < 4; c++) {
                        for (int r = 0; r < 3; r++) {
                            binding.invBindingPose._columns[c][r] = m.m[c * 4 + r];
                        }
                    }
                    binding.bone = { jointNodeId, (int32_t) s._skeleton, 0 , 0};

                    modelDraw->_skinJointBindings.emplace_back(binding);
                }
            }

            // add a dummy joint binding to have a valid buffer even if not used
            if (modelDraw->_skinJointBindings.empty())
            {
                modelDraw->_skinJointBindings.emplace_back(ModelSkinJointBinding());
            }
            // skin buffer
            BufferInit skinBufferInit;
            int32_t skinJointElementPerStruct = 4;
            skinBufferInit.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
            skinBufferInit.bufferSize = modelDraw->_skinJointBindings.size() * sizeof(ModelSkinJointBinding);
            skinBufferInit.hostVisible = true; // TODO Change this to immutable and initialized value
            skinBufferInit.firstElement = 0;
            skinBufferInit.numElements = modelDraw->_skinJointBindings.size() * skinJointElementPerStruct;
            skinBufferInit.structStride = sizeof(ModelSkinJointBinding) / skinJointElementPerStruct;

            auto sbresourceBuffer = device->createBuffer(skinBufferInit);
            memcpy(sbresourceBuffer->_cpuMappedAddress, modelDraw->_skinJointBindings.data(), skinBufferInit.bufferSize);

            modelDraw->_skinBuffer = sbresourceBuffer;
        }

        // Model local bound is the containing box for all the local items of the model
        core::aabox3 model_aabb;
        for (const auto& i : modelDraw->_localItems) {
            if (i.shape != MODEL_INVALID_INDEX) {
                auto nodeIdx = i.node;
                core::mat4x3 transform = modelDraw->_localNodeTransforms[nodeIdx];
                nodeIdx = modelDraw->_localNodeParents[nodeIdx];
                while(nodeIdx != INVALID_NODE_ID) {
                    transform = core::mul(modelDraw->_localNodeTransforms[nodeIdx], transform);
                    nodeIdx = modelDraw->_localNodeParents[nodeIdx];
                }

                const auto& s = modelDraw->_shapes[i.shape];
                core::aabox3 shape_aabb = modelDraw->_partAABBs[s.partOffset];
                for (int p = 1; p < s.numParts; ++p) {
                    shape_aabb = core::aabox3::fromBound(shape_aabb, modelDraw->_partAABBs[p + s.partOffset]);
                };
                shape_aabb = core::aabox_transformFrom(transform, shape_aabb);
                model_aabb = core::aabox3::fromBound(model_aabb, shape_aabb);
            }
        }
        modelDraw->_bound = model_aabb;


        auto [clipData, clips] = Key::createClipsFromGLTF(*model);

        
        modelDraw->_animations = std::make_shared<graphics::Key> ();
        modelDraw->_animations->_data = std::move(clipData);
        modelDraw->_animations->_clips = std::move(clips);

        return modelDraw;
    }

   void ModelDrawFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        const graphics::ScenePointer& scene,
        graphics::ModelDraw& model)
    {
       // It s time to create a descriptorSet that matches the expected pipeline descriptor set
        // then we will assign a uniform buffer in it
       graphics::DescriptorSetInit descriptorSetInit{
           _pipeline->getRootDescriptorLayout(),
           1,
           true
       };
       auto descriptorSet = device->createDescriptorSet(descriptorSetInit);
       model._descriptorSet = descriptorSet;

       // Assign the Camera UBO just created as the resource of the descriptorSet
       graphics::SamplerInit samplerInit{};
       auto sampler = device->createSampler(samplerInit);

       samplerInit._filter = graphics::Filter::MIN_MAG_LINEAR_MIP_POINT;
       auto samplerL = device->createSampler(samplerInit);

       graphics::DescriptorObjects descriptorObjects = {
            { graphics::DescriptorType::RESOURCE_BUFFER, model.getPartBuffer()},
            { graphics::DescriptorType::RESOURCE_BUFFER, model.getIndexBuffer() },
            { graphics::DescriptorType::RESOURCE_BUFFER, model.getVertexBuffer() },
            { graphics::DescriptorType::RESOURCE_BUFFER, model.getVertexAttribBuffer() },
            { graphics::DescriptorType::RESOURCE_BUFFER, model.getSkinBuffer() },
            { graphics::DescriptorType::RESOURCE_BUFFER, model.getMaterialBuffer() },
            { graphics::DescriptorType::RESOURCE_TEXTURE, model.getAlbedoTexture() },
            { sampler },
            { samplerL }
       };
       device->updateDescriptorSet(descriptorSet, descriptorObjects);


       auto numVertices = model.getVertexBuffer()->numElements();
       auto numIndices = model.getIndexBuffer()->numElements();
       auto vertexStride = model.getVertexBuffer()->_init.structStride;
       auto numParts = model.getPartBuffer()->numElements();
       auto numMaterials = model.getMaterialBuffer()->numElements();

       // NUmber of nodes in the model
       auto numNodes = model._localNodeTransforms.size();

       auto pipeline = this->_pipeline;
       auto albedoTex = model.getAlbedoTexture();

       // And now a render callback where we describe the rendering sequence
       graphics::DrawObjectCallback drawCallback = [descriptorSet, pipeline, albedoTex](
           const NodeID node, RenderArgs& args) {
            
            if (albedoTex && albedoTex->needUpload()) {
                args.batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::SHADER_RESOURCE, graphics::ResourceState::COPY_DEST, albedoTex);
                args.batch->uploadTexture(albedoTex);
                args.batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::COPY_DEST, graphics::ResourceState::SHADER_RESOURCE, albedoTex);
            }

            args.batch->bindPipeline(pipeline);

            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, args.viewPassDescriptorSet);
            args.batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet);
       };
       model._drawcall = drawCallback;
       model._drawID = scene->createDraw(model).id();

       if (model._animations && model._animations->_clips.size()) {
           KeyAnim anim = { model._animations };
           model._animID = scene->createAnim(anim).id();
       }

       auto uniforms = model.getUniforms();

       // one draw per part
       DrawIDs drawables;
       for (int d = 0; d < model._partAABBs.size(); ++d) {
           ModelDrawPart part;
           part._bound = model._partAABBs[d];

            auto partNumIndices = model._parts[d].numIndices;
           // And now a render callback where we describe the rendering sequence
            part._drawcall = [d, uniforms, partNumIndices, numNodes, numParts, numMaterials, descriptorSet, pipeline](
               const NodeID node, RenderArgs& args) {
                   ModelObjectData odata{ (uint32_t)node, (uint32_t)d, (uint32_t)numNodes, (uint32_t)numParts, (uint32_t)numMaterials, (uint32_t)uniforms->makeDrawMode() };
                   args.batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 0, sizeof(ModelObjectData), (const uint8_t*)&odata);
                   args.batch->draw(partNumIndices, 0);
           };

           auto partDraw = scene->createDraw(part);
           drawables.emplace_back(partDraw.id());
       }

       model._partDraws = drawables;

    }

   graphics::ItemIDs ModelDrawFactory::createModelParts(
                    const graphics::NodeID root,
                    const graphics::ScenePointer& scene,
                    graphics::ModelDraw& model) {
 
       auto rootNode = scene->createNode({
           .parent = root,
           .localTransform = core::mat4x3(),
           .name = model._name });

       // Allocating the new instances of scene::nodes, one per local node
       auto modelNodes = scene->createNodeBranch({
           .rootParent = rootNode.id(),
           .parentOffsets = model._localNodeParents,
           .localTransforms = model._localNodeTransforms,
           .names = model._localNodeNames });

        // Allocate the new scene::items combining the localItem's node with every shape parts
        graphics::ItemIDs items;
        
        // first item is the model draw itself
        Scene::ItemInit init = {};
        init.node = rootNode.id();
        init.draw = model._drawID;
        init.anim = model._animID;
        init.name = model._name;
        auto rootItemId = scene->createItem(init).id();
        items.emplace_back(rootItemId);

        for (const auto& li : model._localItems) {
            if (li.shape != MODEL_INVALID_INDEX) {
                const auto& s = model._shapes[li.shape];
                for (uint32_t si = 0; si < s.numParts; ++si) {
                    items.emplace_back(scene->createItem({
                        .node= modelNodes[li.node],
                        .draw= model._partDraws[si + s.partOffset], 
                        .group = rootItemId,
                        .name = model._localNodeNames[li.node],
                        }).id());
                }
            }
            if (li.camera != MODEL_INVALID_INDEX) {
                
            }
        }

        return items; 
   }


} // !namespace graphics