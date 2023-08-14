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
#include <core/math/Math3D.h>
#include <core/math/CameraTransform.h>
#include <document/dllmain.h>
#include <document/Image.h>

namespace document
{
namespace model {

    using Index = uint32_t;
    constexpr Index INVALID_INDEX{(uint32_t)-1};
    using IndexArray = std::vector<Index>;

    class Scene {
    public:
        IndexArray _nodes;
    };
    using SceneArray = std::vector<Scene>;

    class Node {
    public:
        std::string _name;

        core::mat4x3 _transform;

        Index _parent{ INVALID_INDEX };
        IndexArray _children;
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
        Index _tangents{ INVALID_INDEX };
        Index _texcoords{ INVALID_INDEX };
        Index _joints{ INVALID_INDEX };
        Index _weights{ INVALID_INDEX };

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
        float _roughnessFactor{ 1.0f };
        Index _baseColorTexture{ INVALID_INDEX };
        Index _normalTexture{ INVALID_INDEX };
        Index _roughnessMetallicTexture{ INVALID_INDEX };
        Index _occlusionTexture{ INVALID_INDEX };
        Index _emissiveTexture{ INVALID_INDEX };
    };
    using MaterialArray = std::vector<Material>;


    class Camera {
    public:
        std::string _name;
        core::Projection _projection;
    };
    using CameraArray = std::vector<Camera>;

    class Item {
    public:
        Index _node{ INVALID_INDEX };
        Index _mesh{ INVALID_INDEX };
        Index _skin{ INVALID_INDEX };
        Index _camera{ INVALID_INDEX };
    };
    using ItemArray = std::vector<Item>;

    class ImageReference {
    public:
        std::string _name;
        std::vector<uint8_t> _data;
        std::string _mimeType;
    };
    using ImageReferenceArray = std::vector<ImageReference>;
    using ImageArray = std::vector<Image>;

    class Sampler {
    public:
        enum Filter : uint8_t {
            NEAREST = 0,
            LINEAR,
            NEAREST_MIPMAP_NEAREST,
            LINEAR_MIPMAP_NEAREST,
            NEAREST_MIPMAP_LINEAR,
            LINEAR_MIPMAP_LINEAR
        };
        enum Wrap : uint8_t {
            CLAMP_TO_EDGE = 0,
            MIRRORED_REPEAT,
            REPEAT
        };

        std::string _name;
        Filter magFilter{ NEAREST };
        Filter minFilter{ NEAREST };
        Wrap wrapS{ CLAMP_TO_EDGE };
        Wrap wrapT{ CLAMP_TO_EDGE };
    };
    using SamplerArray = std::vector<Sampler>;

    class Texture {
    public:
        std::string _name;
        Index _image;
        Index _sampler;
    };
    using TextureArray = std::vector<Texture>;

    class AnimationChannelTarget {
    public:
        Index _node = -1;

        enum Path : uint8_t {
            TRANSLATION = 0,
            ROTATION,
            SCALE,
            WEIGHTS,
            NONE
        };
        Path _path = TRANSLATION;
    };

    class AnimationChannel {
    public:
        Index _sampler = -1;
        AnimationChannelTarget _target;

    };
    using AnimationChannelArray = std::vector<AnimationChannel>;

    class AnimationSampler {
    public:
        Index _input = -1;
        Index _output = -1;
        enum Interpolation : uint8_t {
            LINEAR = 0,
            STEP,
            CUBICSPLINE,
            NONE
        };
        Interpolation _interpolation = LINEAR;
    };
    using AnimationSamplerArray = std::vector<AnimationSampler>;

    class Animation {
    public:
        std::string _name;
        AnimationChannelArray _channels;
        AnimationSamplerArray _samplers;
    };
    using AnimationArray = std::vector<Animation>;

    class Skin {
    public:
        std::string _name;
        Index _inverseBindMatrices{ INVALID_INDEX };
        Index _skeleton{ INVALID_INDEX };
        IndexArray _joints;
        
    };
    using SkinArray = std::vector<Skin>;

    class DOCUMENT_API Model {
    public:
        static std::unique_ptr<Model> createFromGLTF(const std::string& filename);

        // the name of this model
        std::string _name;

        SceneArray _scenes;
        NodeArray _nodes;

        ItemArray _items;

        BufferArray _buffers;
        BufferViewArray _bufferViews;

        MeshArray _meshes;
        PrimitiveArray _primitives;
        AccessorArray _accessors;

        MaterialArray _materials;

        CameraArray _cameras;

        ImageReferenceArray _imageReferences;
        ImageArray _images;

        SamplerArray _samplers;
        TextureArray _textures;

        AnimationArray _animations;
        SkinArray _skins;

    };

    template <typename T>
    T FetchBuffer(int elementIndex, const Accessor& accessor, const BufferView& bufferView, const Buffer& buffer) {
        T val;
        if (elementIndex >= 0 && elementIndex < accessor._elementCount) {
            auto byteStride = (bufferView._byteStride ? bufferView._byteStride : document::model::componentTypeSize(accessor._componentType) * document::model::elementTypeComponentCount(accessor._elementType));
            const uint8_t* data = buffer._bytes.data() + bufferView._byteOffset + accessor._byteOffset + byteStride * elementIndex;
            return *(const T*)data;
        }
        return val;
    }

    template <typename T>
    T FetchBuffer(int elementIndex, int accessorId, const Model& model) {
        const auto& accessor = model._accessors[accessorId];
        const auto& bufferView = model._bufferViews[accessor._bufferView];
        const auto& buffer = model._buffers[bufferView._buffer];
        return FetchBuffer<T>(elementIndex, accessor, bufferView, buffer);
    }
}
    using Model = model::Model;
    using ModelPointer = std::shared_ptr<Model>;

}