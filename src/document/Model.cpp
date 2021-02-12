// Model.cpp
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
#include "Model.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>
#include <algorithm>

#include <core/json/json.h>
#include <core/Log.h>

using namespace document;
using namespace model;
using namespace core;

json check(const json& j, const std::string& k) {
    if (j.contains(k)) {
        return j[k];
    }
    else {
        return json();
    }
}

std::tuple<NodeArray, ItemArray> parseNodes(const json& gltf_nodes) {
    // gltf combine in the node both Node and Item concepts
    NodeArray nodes;
    ItemArray items;

    if (gltf_nodes.is_array()) {
        for (const auto& n : gltf_nodes) {
            Node node;

            const auto& name = check(n, "name");
            if (name.is_string()) {
                node._name = name.get<std::string>();
            }

            const auto& x = check(n, "matrix");
            if (x.is_array()) {

                for (uint32_t c = 0; c < 4; ++c) {
                    for (uint32_t r = 0; r < 3; ++r) {
                        node._transform._columns[c][r] = x[c * 4 + r].get<float>();
                    }
                }

            } else {
                vec3 T{ 0.0f };
                rotor3 R;
                vec3 S{1.0f};

                const auto& t = check(n, "translation");
                const auto& r = check(n, "rotation");
                const auto& s = check(n, "scale");

                if (t.is_array()) {
                    T.x = t[0];
                    T.y = t[1];
                    T.z = t[2];
                }
                if (r.is_array() ) {
                    R.a = r[3];
                    R.b.xy = r[2];
                    R.b.xz = r[1];
                    R.b.yz = r[0];
                }
                if (s.is_array()) {
                    S.x = s[0];
                    S.y = s[1];
                    S.z = s[2];
                }

                node._transform = translation_rotation(T, R);
            }

            const auto& children = check(n, "children");
            if (children.is_array()) {
                for (const auto& c : children) {
                    node._children.emplace_back(c.get<uint32_t>());
                }
            }

            const auto& m = check(n, "mesh");
            if (m.is_number()) {
                node._index = m.get<Index>();
            }
            nodes.emplace_back(node);
        }
    }
    
    // Assign the parent fields by reparsing the nodes
    // And build the items on that 2nd pass
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        auto& n = nodes[i];
        for (auto c : n._children) {
            nodes[c]._parent = i;
        }

        // build item
        if (n._index != INVALID_INDEX) {
            items.emplace_back(Item{i, n._index});
        }
    }

    return {std::move(nodes), std::move(items)};
}

struct GLBHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t length;

    bool isValid() const { return magic == 0x46546C67; }
};

std::vector<uint8_t> parseBinary(const uint64_t byteLength, const std::filesystem::path& glb_path) {
    std::vector<uint8_t> bytes;
    if (std::filesystem::exists(glb_path)) {
        auto fileSize = std::filesystem::file_size(glb_path);
        auto readSize = std::min(fileSize, byteLength);
        
        std::ifstream  src(glb_path.c_str(), std::ios::binary | std::ios::in);
        if(!src.fail()) {
            bytes.resize(readSize, 0);
            // read header first 12 bytes
            src.read(reinterpret_cast<char*> (bytes.data()), readSize);
        }
        src.close();
    }
    return std::move(bytes);
}

/*
   base64.cpp and base64.h
   Copyright (C) 2004-2008 René Nyffenegger
   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.
   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:
   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.
   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.
   3. This notice may not be removed or altered from any source distribution.
   René Nyffenegger rene.nyffenegger@adp-gmbh.ch
*/
static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::vector<uint8_t> base64_decode(std::string const& encoded_string) {
    int in_len = static_cast<int>(encoded_string.size());
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    while (in_len-- && (encoded_string[in_] != '=') &&
        is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_];
        in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] =
                static_cast<unsigned char>(base64_chars.find(char_array_4[i]));

            char_array_3[0] =
                (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] =
                ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++) ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++) char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] =
            static_cast<unsigned char>(base64_chars.find(char_array_4[j]));

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] =
            ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }

    std::vector<uint8_t> out;
    if (!ret.empty()) {
        out.resize(ret.size());
        std::copy(ret.begin(), ret.end(), out.begin());
    }

    return std::move(out);
}

std::tuple<std::string, std::vector<uint8_t>> parseURI(const uint64_t byteLength, const std::string& uri, const std::filesystem::path& model_path_root) {
   std::string urim;  
   std::vector<uint8_t> bytes;

   const std::string embedded_header = "data:application/octet-stream;base64,";
   if (uri.find(embedded_header) == 0) {
       urim = embedded_header;
       bytes = base64_decode(uri.substr(embedded_header.size()));  // cut mime string.
   } else {
       urim = uri;
       auto glb_path = model_path_root / uri;
       bytes = parseBinary(byteLength, glb_path);
   }
   
    return {std::move(urim), std::move(bytes)};
}

BufferArray parseBuffers(const json& gltf_buffers, const std::filesystem::path& model_path_root) {
    BufferArray buffers;
    if (gltf_buffers.is_array()) {
        for (const auto& b : gltf_buffers) {
            Buffer buffer;

            const auto& bl = check(b, "byteLength");
            if (bl.is_number_integer()) {
                buffer._byteLength = bl.get<uint64_t>();
            }

            const auto& uri = check(b, "uri");
            if (uri.is_string()) {
                const auto& uri_string = uri.get<std::string>();

                std::tie(buffer._uri, buffer._bytes) = parseURI(buffer._byteLength, uri_string, model_path_root);
            }

            buffers.emplace_back(buffer);
        }
        
    }

    return std::move(buffers);
}

BufferViewArray parseBufferViews(const json& gltf_bufferViews) {
    BufferViewArray bufferViews;
    if (gltf_bufferViews.is_array()) {
        for (const auto& b : gltf_bufferViews) {

            BufferView view;

            const auto& buffer = check(b, "buffer");
            if (buffer.is_number_integer()) {
                view._buffer = buffer.get<uint32_t>();
            }

            const auto& bo = check(b, "byteOffset");
            if (bo.is_number_integer()) {
                view._byteOffset = bo.get<uint64_t>();
            }
            const auto& bl = check(b, "byteLength");
            if (bl.is_number_integer()) {
                view._byteLength = bl.get<uint64_t>();
            }
            const auto& bs = check(b, "byteStride");
            if (bs.is_number_integer()) {
                view._byteStride = bs.get<uint32_t>();
            }

            bufferViews.emplace_back(view);
        }
    }

    return std::move(bufferViews);
}



AccessorArray parseAccessors(const json& gltf_accessors) {
    AccessorArray accessors;

    if (gltf_accessors.is_array()) {
        for (const auto& a : gltf_accessors) {
            Accessor access;

            const auto& bv = check(a, "bufferView");
            if (bv.is_number_integer()) {
                access._bufferView = bv.get<uint32_t>();
            }

            const auto& bo = check(a, "byteOffset");
            if (bo.is_number_integer()) {
                access._byteOffset = bo.get<uint64_t>();
            }

            const auto& ct = check(a, "componentType");
            if (ct.is_number_integer()) {
                uint32_t componentType = ct.get<uint32_t>();

                switch (componentType) {
                case 5120: access._componentType = ComponentType::Int8; break;
                case 5121: access._componentType = ComponentType::UInt8; break;
                case 5122: access._componentType = ComponentType::Int16; break;
                case 5123: access._componentType = ComponentType::UInt16; break;
                case 5125: access._componentType = ComponentType::UInt32; break;
                case 5126: access._componentType = ComponentType::Float; break;
                }
            }
            const auto& cc = check(a, "count");
            if (cc.is_number_integer()) {
                access._elementCount = cc.get<uint32_t>();
            }

            const auto& t = check(a, "type");
            uint32_t numComponents = 0;
            if (t.is_string()) {
                const auto& type = t.get<std::string>();
                if (type == "SCALAR") { 
                    access._elementType = ElementType::Scalar;
                } else if (type == "VEC2") {
                    access._elementType = ElementType::Vec2;
                } else if (type == "VEC3") {
                    access._elementType = ElementType::Vec3;
                } else if (type == "VEC4") {
                    access._elementType = ElementType::Vec4;
                } else if (type == "MAT2") {
                    access._elementType = ElementType::Mat2;
                } else if (type == "MAT3") {
                    access._elementType = ElementType::Mat3;
                } else if (type == "MAT4") {
                    access._elementType = ElementType::Mat4;
                }

                numComponents = elementTypeComponentCount(access._elementType);
            }

            const auto count = core::min(numComponents, 3);
            core::vec3 min_value{ FLT_MAX };
            core::vec3 max_value{ -FLT_MAX };
            const auto& lmin = check(a, "min");
            if (lmin.is_array()) {
                for (int c = 0; c < count; ++c )
                    min_value[c] = (*(lmin.begin() + c)).get<float>();
            }

            const auto& lmax = check(a, "max");
            if (lmax.is_array()) {
                for (int c = 0; c < count; ++c)
                    max_value[c] = (*(lmax.begin() + c)).get<float>();
            }

            access._aabb = core::aabox3::fromMinMax(min_value, max_value);

            accessors.emplace_back(access);
        }
    }

    return std::move(accessors);
}

std::tuple<MeshArray, PrimitiveArray> parseMeshes(const json& gltf_meshes) {
    MeshArray meshes;
    PrimitiveArray primitives;

    if (gltf_meshes.is_array()) {
        for (const auto& m : gltf_meshes) {
            Mesh mesh;

            const auto& gltf_p = check(m, "primitives");
            if (gltf_p.is_array()) {

                mesh._primitiveStart = primitives.size();
                mesh._primitiveCount = 0;

                for (const auto& p : gltf_p) {
                    Primitive prim;
                
                    const auto& a = check(p, "attributes");
                    if (a.is_object()) {
                        
                        const auto& pos = check(a, "POSITION");
                        if (pos.is_number_integer()) {
                            prim._positions = pos.get<uint32_t>();
                        }

                        const auto& nor = check(a, "NORMAL");
                        if (nor.is_number_integer()) {
                            prim._normals = nor.get<uint32_t>();
                        }
                    }

                    const auto& ind = check(p, "indices");
                    if (ind.is_number_integer()) {
                        prim._indices = ind.get<uint32_t>();
                    }

                    const auto& mat = check(p, "material");
                    if (mat.is_number_integer()) {
                        prim._material = mat.get<uint32_t>();
                    }

                    primitives.emplace_back(prim);
                    mesh._primitiveCount ++;
                }
            }

            meshes.emplace_back(mesh);
        }
    }

    return std::make_tuple(meshes, primitives);
}

MaterialArray parseMaterials(const json& gltf_materials) {
    MaterialArray materials;

    if (gltf_materials.is_array()) {
        for (const auto& m : gltf_materials) {
            Material material;

            const auto& pbrMetallicRoughness = check(m, "pbrMetallicRoughness");
            if (pbrMetallicRoughness.is_object()) {

                const auto& color = check(pbrMetallicRoughness, "baseColorFactor");
                if (color.is_array()) {
                    for (int c = 0; c < color.size(); ++c)
                        material._baseColor[c] = (*(color.begin() + c)).get<float>();
                }

                const auto& metallic = check(pbrMetallicRoughness, "metallicFactor");
                if (metallic.is_number()) {
                    material._metallicFactor = metallic.get<float>();
                }


                const auto& roughness = check(pbrMetallicRoughness, "roughnessFactor");
                if (roughness.is_number()) {
                    material._roughnessFactor = roughness.get<float>();
                }
            }

            materials.emplace_back(material);
        }
    }

    return materials;
}

std::unique_ptr<Model> parseModel(const json& gltf, const std::filesystem::path& model_path_root) {
    auto model = std::make_unique<Model>();
    
    
    std::tie( model->_nodes, model->_items) = parseNodes(check(gltf, "nodes"));
    

    model->_buffers = parseBuffers(check(gltf, "buffers"), model_path_root);
    model->_bufferViews = parseBufferViews(check(gltf, "bufferViews"));


    model->_accessors = parseAccessors(check(gltf, "accessors"));
    std::tie( model->_meshes, model->_primitives) = parseMeshes(check(gltf, "meshes"));

    model->_materials = parseMaterials(check(gltf, "materials"));

    return model;
}

std::unique_ptr<Model> Model::createFromGLTF(const std::string& filename) {
    if (filename.empty()) {
        return nullptr;
    }
    auto model_path = std::filesystem::absolute(std::filesystem::path(filename));
    if (!std::filesystem::exists(model_path)) {
        picoLog() << "model file " << model_path.string() << " doesn't exist\n";
        return nullptr;
    }
    auto model_path_root = model_path.parent_path();

    // let's try to open the file
    std::ifstream file(model_path.string(), std::ifstream::in);
    std::string content((std::istreambuf_iterator<char>(file)),
        (std::istreambuf_iterator<char>()));
    file.close();

    core::json gltf_data;
    try {
        gltf_data = core::json::parse(content);
        if (gltf_data.is_object()) {
            return parseModel(gltf_data, model_path_root);
        }
        else {
            picoLog() << "gltf_data file " << model_path.string() << " doesn't have root object\n";
            return nullptr;
        }
    }
    catch (...) {
        picoLog() << "gltf_data file " << model_path.string() << " is not valid json\n";
        return nullptr;
    }
}
