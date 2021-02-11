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

#include <string>
#include <vector>
#include <memory>
#include <core/math/LinearAlgebra.h>
#include <document/dllmain.h>

namespace document
{
namespace model {

    using Index = uint32_t;
    constexpr Index INVALID_INDEX{(uint32_t)-1};
    using IndexArray = std::vector<Index>;

    class Node {
    public:
        std::string _name;

        core::mat4x3 _transform;

        Index _parent{ INVALID_INDEX };
        IndexArray _children;

        Index _index{ INVALID_INDEX };
    };
    using NodeArray = std::vector<Node>;

    class Buffer {
    public:
        uint64_t _byteLength{ 0 };
        std::string _uri;
        std::vector<uint8_t> _bytes;
    };
    using BufferArray = std::vector<Buffer>;

    class BufferView {
    public:
        Index _buffer{ INVALID_INDEX };
        uint64_t _byteOffset{ 0 };
        uint64_t _byteLength{ 0 };
        uint32_t _byteStride{ 0 };
    };
    using BufferViewArray = std::vector<BufferView>;

    enum class ComponentType : uint8_t {
        UInt8 = 0,
        UInt16,
        UInt32,
        UInt64,
        Int8,
        Int16,
        Int32,
        Int64,
        Float,
    };
    constexpr uint32_t componentTypeSize(ComponentType t) {
        constexpr uint32_t SIZES[9] = {
            1, 2, 4, 8, 1, 2, 4, 8, 4
        };
        return SIZES[(uint8_t) t];
    }
    constexpr uint32_t componentTypeInt32Mask(ComponentType t) {
        constexpr uint32_t MASKS[9] = {
            0x000000FF, 0x0000FFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x000000FF, 0x0000FFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
        };
        return MASKS[(uint8_t)t];
    }
    enum class ElementType : uint8_t {
        Scalar = 0,
        Vec2,
        Vec3,
        Vec4,
        Mat2,
        Mat3,
        Mat4,
    };
    constexpr uint32_t elementTypeComponentCount(ElementType t) {
        constexpr uint32_t COUNTS[7] = {
            1, 2, 3, 4, 4, 9, 16
        };
        return COUNTS[(uint8_t)t];
    }
    class Accessor {
    public:
        Index _bufferView { INVALID_INDEX };
        uint64_t _byteOffset{ 0 };
        uint32_t _elementCount { 0 };

        ComponentType _componentType { ComponentType::UInt8 };
        ElementType _elementType{ ElementType::Scalar };

        core::aabox3 _aabb;
    };

    using AccessorArray = std::vector<Accessor>;

    class Primitive {
    public:
        Index _positions{ INVALID_INDEX };
        Index _normals{ INVALID_INDEX };

        Index _indices{ INVALID_INDEX };
        Index _material{ INVALID_INDEX };
    };
    using PrimitiveArray = std::vector<Primitive>;

    class Mesh {
    public:
        std::string _name;
        Index _primitiveStart{ INVALID_INDEX };
        uint32_t _primitiveCount{ 0 };
    };
    using MeshArray = std::vector<Mesh>;


    class Material {
    public:
        std::string _name;
        core::vec4 _baseColor{ 0.5f, 0.5f, 0.5f, 1.0f };
        float _metallicFactor{ 1.0f };
        float _roughnessFactor{ 0.0f };
    };
    using MaterialArray = std::vector<Material>;


    class Item {
    public:
        Index _node{ INVALID_INDEX };
        Index _mesh{ INVALID_INDEX };
    };
    using ItemArray = std::vector<Item>;


    class Model;

    class DOCUMENT_API Model {
    public:
        static std::unique_ptr<Model> createFromGLTF(const std::string& filename);


        
        NodeArray _nodes;

        ItemArray _items;

        BufferArray _buffers;
        BufferViewArray _bufferViews;

        MeshArray _meshes;
        PrimitiveArray _primitives;
        AccessorArray _accessors;

        MaterialArray _materials;
    };
}
    using Model = model::Model;
    using ModelPointer = std::shared_ptr<Model>;

}