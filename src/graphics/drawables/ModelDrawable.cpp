// ModelDrawable.cpp
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
#include "ModelDrawable.h"

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
#include "render/Drawable.h"
#include "render/Viewport.h"
#include "render/Mesh.h"

#include "ModelPart_vert.h"
#include "ModelPart_frag.h"

//using namespace view3d;
namespace graphics
{

    ModelDrawableFactory::ModelDrawableFactory() :
        _sharedUniforms(std::make_shared<ModelDrawableUniforms>()) {

    }
    ModelDrawableFactory::~ModelDrawableFactory() {

    }

    // Custom data uniforms
    struct ModelObjectData {
        uint32_t nodeID{0};
        uint32_t partID{0};
        uint32_t numNodes{ 0 };
        uint32_t numParts{ 0 };
        uint32_t numMaterials{ 0 };
    };

    void ModelDrawableFactory::allocateGPUShared(const graphics::DevicePointer& device) {

        // Let's describe the pipeline Descriptors layout
        graphics::DescriptorLayouts descriptorLayouts{
            { graphics::DescriptorType::UNIFORM_BUFFER, graphics::ShaderStage::VERTEX, 0, 1},
            { graphics::DescriptorType::PUSH_UNIFORM, graphics::ShaderStage::ALL_GRAPHICS, 1, sizeof(ModelObjectData) >> 2},
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 0, 1}, // Node Transform
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::ALL_GRAPHICS, 1, 1}, // Part
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 2, 1}, // Index
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 3, 1}, // Vertex
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::VERTEX, 4, 1}, // Attrib
            { graphics::DescriptorType::RESOURCE_BUFFER, graphics::ShaderStage::PIXEL, 5, 1},  // Material
            { graphics::DescriptorType::RESOURCE_TEXTURE, graphics::ShaderStage::PIXEL, 0, 1},  // Albedo Texture
            { graphics::DescriptorType::SAMPLER, graphics::ShaderStage::PIXEL, 0, 1},
        };

        graphics::DescriptorSetLayoutInit descriptorSetLayoutInit{ descriptorLayouts };
        auto descriptorSetLayout = device->createDescriptorSetLayout(descriptorSetLayoutInit);

        // And a Pipeline

        // Load shaders (as stored in the resources)
        auto shader_vertex_src = ModelPart_vert::getSource();
        auto shader_pixel_src = ModelPart_frag::getSource();

        // test: create shader
        graphics::ShaderInit vertexShaderInit{ graphics::ShaderType::VERTEX, "main", "", shader_vertex_src, ModelPart_vert::getSourceFilename() };
        graphics::ShaderPointer vertexShader = device->createShader(vertexShaderInit);

        graphics::ShaderInit pixelShaderInit{ graphics::ShaderType::PIXEL, "main", "", shader_pixel_src, ModelPart_frag::getSourceFilename() };
        graphics::ShaderPointer pixelShader = device->createShader(pixelShaderInit);

        graphics::ProgramInit programInit{ vertexShader, pixelShader };
        graphics::ShaderPointer programShader = device->createProgram(programInit);

        graphics::GraphicsPipelineStateInit pipelineInit{
                    programShader,
                    StreamLayout(),
                    graphics::PrimitiveTopology::TRIANGLE,
                    descriptorSetLayout,
                    RasterizerState().withCullBack(),
                    true, // enable depth
                    BlendState()
        };
        _pipeline = device->createGraphicsPipelineState(pipelineInit);
    }


    size_t hash(ModelVertex& v) {
            return      *reinterpret_cast<uint64_t*>((uint64_t*)&v.px)
                    | *reinterpret_cast<uint64_t*>((uint64_t*)&v.pz);
    }
    size_t hash(ModelVertexAttrib& a) {
            return  *reinterpret_cast<uint64_t*>((uint64_t*)&a);
    }

    graphics::ModelDrawable* ModelDrawableFactory::createModel(const graphics::DevicePointer& device, const document::ModelPointer& model) {

        auto modelDrawable = new graphics::ModelDrawable();

        // Define the local nodes used by the model with the original transforms and the parents
        modelDrawable->_localNodeTransforms.reserve(model->_nodes.size());
        modelDrawable->_localNodeParents.reserve(model->_nodes.size());
        for (const auto& n : model->_nodes) {
            modelDrawable->_localNodeTransforms.emplace_back(n._transform);
            modelDrawable->_localNodeParents.emplace_back(n._parent);
        }

        // Define the items
        modelDrawable->_localItems.reserve(model->_items.size());
        for (const auto& si : model->_items) {
            modelDrawable->_localItems.emplace_back(ModelItem{si._node, si._mesh, si._camera });
        }

        // Define the shapes
        modelDrawable->_shapes.reserve(model->_meshes.size());
        for (const auto& m : model->_meshes) {
            modelDrawable->_shapes.emplace_back(ModelShape{ m._primitiveStart, m._primitiveCount });
        }

        // Define the cameras
        modelDrawable->_localCameras.reserve(model->_cameras.size());
        for (const auto& cam : model->_cameras) {
            modelDrawable->_localCameras.emplace_back(ModelCamera{ cam._projection });
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

        using LookupEdge = struct { uint32_t index; uint32_t t0; uint32_t t1{ (uint32_t) -1 }; };
        using LookupEdgeIndexedTable = std::unordered_map<size_t, LookupEdge >;
        LookupEdgeIndexedTable indexedEdgeMap;


        bool first = true;
        for (const auto& p : model->_primitives) {

            const auto& indexAccess = model->_accessors[p._indices];
            const auto& indexView = model->_bufferViews[indexAccess._bufferView];
            const auto& indexBuffer = model->_buffers[indexView._buffer];
            
            // Index accessor
            std::vector<ModelIndex> partIndices;
            auto indexStride = (indexView._byteStride ? indexView._byteStride : document::model::componentTypeSize(indexAccess._componentType));
            uint32_t indexMask = document::model::componentTypeInt32Mask(indexAccess._componentType);
            for (uint32_t i = 0; i < indexAccess._elementCount; ++i) {
                auto index = (*(uint32_t*)(indexBuffer._bytes.data() + indexView._byteOffset + indexAccess._byteOffset + indexStride * i)) & indexMask;
                partIndices.emplace_back(index);
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

            std::vector<ModelVertex> partVerts;
            std::vector<ModelVertexAttrib> partAttribs;

            for (uint32_t i = 0; i < partIndices.size(); ++i) {
                uint32_t index = partIndices[i];
                auto vp = positionGetter(index);
                auto vn = normalGetter(index);
                auto vt = texcoordGetter(index);

                ModelVertex v{ vp.x, vp.y, vp.z, vn };
                partVerts.emplace_back(v);

                ModelVertexAttrib a{ vt.x, vt.y, 0, 0 };
                partAttribs.emplace_back(a);

                //hash a key on vector pos and normal
                size_t kv = hash(v);
                size_t ka = hash(a);

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
                        picoAssert( (((int32_t) e_bucket->second.t1) == (-1)), "we have a problem, more than 2 triangles for that edge ???");
                        
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

            ModelPart part{ partIndices.size(), (uint32_t)index_buffer.size(), 0, 0, p._material, partEdges.size(), 0  };
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

        modelDrawable->_edges = std::move(edge_buffer);
        modelDrawable->_edgeBuffer = ebuniformBuffer;

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

        modelDrawable->_faces = std::move(face_buffer);
        modelDrawable->_faceBuffer = fbuniformBuffer;


        modelDrawable->_uniforms = _sharedUniforms;
        modelDrawable->_indexBuffer = ibresourceBuffer;
        modelDrawable->_vertexBuffer = vbresourceBuffer;
        modelDrawable->_vertexAttribBuffer = vabresourceBuffer;
        modelDrawable->_partBuffer = pbuniformBuffer;

        // Also need a version of the mesh and parts and their bound on the cpu side
        modelDrawable->_vertices = std::move(vertex_buffer);
        modelDrawable->_vertex_attribs = std::move(vertex_attrib_buffer);
        modelDrawable->_indices = std::move(index_buffer);
        modelDrawable->_parts = std::move(parts);
        modelDrawable->_partAABBs = std::move(partAABBs);

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

        modelDrawable->_materialBuffer = mbresourceBuffer;

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
            
            TextureInit albedoTexInit;
            albedoTexInit.width = maxWidth;
            albedoTexInit.height = maxHeight;
            albedoTexInit.numSlices = numImages;
            albedoTexInit.initData = std::move(pixels);
            auto albedoresourceTexture = device->createTexture(albedoTexInit);

            modelDrawable->_albedoTexture = albedoresourceTexture;
        }
        
        // Model local bound is the containing box for all the local items of the model
        core::aabox3 model_aabb;
        for (const auto& i : modelDrawable->_localItems) {
            if (i.shape != MODEL_INVALID_INDEX) {
                auto nodeIdx = i.node;
                core::mat4x3 transform = modelDrawable->_localNodeTransforms[nodeIdx];
                nodeIdx = modelDrawable->_localNodeParents[nodeIdx];
                while(nodeIdx != INVALID_NODE_ID) {
                    transform = core::mul(modelDrawable->_localNodeTransforms[nodeIdx], transform);
                    nodeIdx = modelDrawable->_localNodeParents[nodeIdx];
                }

                const auto& s = modelDrawable->_shapes[i.shape];
                core::aabox3 shape_aabb = modelDrawable->_partAABBs[s.partOffset];
                for (int p = 1; p < s.numParts; ++p) {
                    shape_aabb = core::aabox3::fromBound(shape_aabb, modelDrawable->_partAABBs[p + s.partOffset]);
                };
                shape_aabb = core::aabox_transformFrom(transform, shape_aabb);
                model_aabb = core::aabox3::fromBound(model_aabb, shape_aabb);
            }
        }
        modelDrawable->_bound = model_aabb;

        return modelDrawable;
    }

   void ModelDrawableFactory::allocateDrawcallObject(
        const graphics::DevicePointer& device,
        const graphics::ScenePointer& scene,
        const graphics::CameraPointer& camera,
        graphics::ModelDrawable& model)
    {
       // It s time to create a descriptorSet that matches the expected pipeline descriptor set
        // then we will assign a uniform buffer in it
       graphics::DescriptorSetInit descriptorSetInit{
           _pipeline->getDescriptorSetLayout()
       };
       auto descriptorSet = device->createDescriptorSet(descriptorSetInit);
       model._descriptorSet = descriptorSet;

       // Assign the Camera UBO just created as the resource of the descriptorSet
       // auto descriptorObjects = descriptorSet->buildDescriptorObjects();
       graphics::DescriptorObject camera_uboDescriptorObject;
       camera_uboDescriptorObject._uniformBuffers.push_back(camera->getGPUBuffer());
       graphics::DescriptorObject transform_rboDescriptorObject;
       transform_rboDescriptorObject._buffers.push_back(scene->_nodes._transforms_buffer);

       graphics::DescriptorObject pb_rboDescriptorObject;
       pb_rboDescriptorObject._buffers.push_back(model.getPartBuffer());
       graphics::DescriptorObject ib_rboDescriptorObject;
       ib_rboDescriptorObject._buffers.push_back(model.getIndexBuffer());
       graphics::DescriptorObject vb_rboDescriptorObject;
       vb_rboDescriptorObject._buffers.push_back(model.getVertexBuffer());
       graphics::DescriptorObject ab_rboDescriptorObject;
       ab_rboDescriptorObject._buffers.push_back(model.getVertexAttribBuffer());

       graphics::DescriptorObject mb_rboDescriptorObject;
       mb_rboDescriptorObject._buffers.push_back(model.getMaterialBuffer());
       graphics::DescriptorObject texDescriptorObject;
       texDescriptorObject._textures.push_back(model.getAlbedoTexture());
       graphics::DescriptorObject samplerDescriptorObject;
       graphics::SamplerInit samplerInit{};
       auto sampler = device->createSampler(samplerInit);
       samplerDescriptorObject._samplers.push_back(sampler);

       graphics::DescriptorObjects descriptorObjects = {
            camera_uboDescriptorObject,
            transform_rboDescriptorObject, 
            pb_rboDescriptorObject,
            ib_rboDescriptorObject,
            vb_rboDescriptorObject,
            ab_rboDescriptorObject,
            mb_rboDescriptorObject,
            texDescriptorObject,
            samplerDescriptorObject
       };
       device->updateDescriptorSet(descriptorSet, descriptorObjects);


       auto numVertices = model.getVertexBuffer()->getNumElements();
       auto numIndices = model.getIndexBuffer()->getNumElements();
       auto vertexStride = model.getVertexBuffer()->_init.structStride;
       auto numParts = model.getPartBuffer()->getNumElements();
       auto numMaterials = model.getMaterialBuffer()->getNumElements();

       // NUmber of nodes in the model
       auto numNodes = model._localNodeTransforms.size();

       auto pipeline = this->_pipeline;
       auto albedoTex = model.getAlbedoTexture();

       // And now a render callback where we describe the rendering sequence
       graphics::DrawObjectCallback drawCallback = [descriptorSet, pipeline, albedoTex](
           const NodeID node,
           const graphics::CameraPointer& camera,
           const graphics::SwapchainPointer& swapchain,
           const graphics::DevicePointer& device,
           const graphics::BatchPointer& batch) {
            
            static bool first{ true };
            if (first) {
                first = false;
                if (albedoTex) {
                    batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::SHADER_RESOURCE, graphics::ResourceState::COPY_DEST, albedoTex);
                    batch->uploadTextureFromInitdata(device, albedoTex);
                    batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::COPY_DEST, graphics::ResourceState::SHADER_RESOURCE, albedoTex);
                }
            }

            batch->bindPipeline(pipeline);
            batch->setViewport(camera->getViewportRect());
            batch->setScissor(camera->getViewportRect());

            batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet);
       };
       model._drawcall = drawCallback;
       model._drawableID = scene->createDrawable(model).id();

       // one drawable per part
       DrawableIDs drawables;
       for (int d = 0; d < model._partAABBs.size(); ++d) {
           auto part = new ModelDrawablePart();
           part->_bound = model._partAABBs[d];

            auto partNumIndices = model._parts[d].numIndices;
           // And now a render callback where we describe the rendering sequence
           graphics::DrawObjectCallback drawCallback = [d, partNumIndices, numNodes, numParts, numMaterials, descriptorSet, pipeline](
               const NodeID node,
               const graphics::CameraPointer& camera,
               const graphics::SwapchainPointer& swapchain,
               const graphics::DevicePointer& device,
               const graphics::BatchPointer& batch) {
               /*    batch->bindPipeline(pipeline);
                   batch->setViewport(camera->getViewportRect());
                   batch->setScissor(camera->getViewportRect());

                   batch->bindDescriptorSet(graphics::PipelineType::GRAPHICS, descriptorSet);
*/
                   ModelObjectData odata{ (int32_t)node, (int32_t)d, numNodes, numParts, numMaterials };
                   batch->bindPushUniform(graphics::PipelineType::GRAPHICS, 1, sizeof(ModelObjectData), (const uint8_t*)&odata);
                   batch->draw(partNumIndices, 0);
           };

           part->_drawcall = drawCallback;

           auto partDrawable = scene->createDrawable(*part);
           drawables.emplace_back(partDrawable.id());
       }

       model._partDrawables = drawables;

    }

   graphics::ItemIDs ModelDrawableFactory::createModelParts(
                    const graphics::NodeID root,
                    const graphics::ScenePointer& scene,
                    graphics::ModelDrawable& model) {
   
        auto rootNode = scene->createNode(core::mat4x3(), root);

        
        // Allocating the new instances of scene::nodes, one per local node
        auto modelNodes = scene->createNodeBranch(rootNode.id(), model._localNodeTransforms, model._localNodeParents);

        // Allocate the new scene::items combining the localItem's node with every shape parts
        graphics::ItemIDs items;
        
        // first item is the model drawable
        items.emplace_back(scene->createItem(rootNode.id(), model._drawableID).id());

        for (const auto& li : model._localItems) {
            if (li.shape != MODEL_INVALID_INDEX) {
                const auto& s = model._shapes[li.shape];
                for (uint32_t si = 0; si < s.numParts; ++si) {
                    items.emplace_back(scene->createItem(modelNodes[li.node], model._partDrawables[si + s.partOffset]).id());
                }
            }
            if (li.camera != MODEL_INVALID_INDEX) {
                
            }
        }

        return items; 
   }


} // !namespace graphics