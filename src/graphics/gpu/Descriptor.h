// Descriptor.h 
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
#pragma once

#include "gpu.h"

#include <vector>

namespace graphics {

    // A descriptor layout for an individual resource
    struct VISUALIZATION_API DescriptorLayout {
        DescriptorType  _type;
        ShaderStage     _shaderStage;
        uint32_t        _binding;
        uint32_t        _count;
    };

    // DescriptorSet Layout
    // N Descriptor Layouts for N sequential resource descriptors in the DescriptorHeap
    using DescriptorSetLayout = std::vector<DescriptorLayout>;

    // Several DescriptorSet Layout
    using DescriptorSetLayouts = std::vector<DescriptorSetLayout>;


    // Root Descriptor Layout
    // 1 DescriptorLayout for push constants
    // N DescriptorSetLayouts for resources
    // 1 DescriptorSetLayout for samplers
    struct VISUALIZATION_API RootDescriptorLayoutInit {
        DescriptorSetLayout  _pushLayout;
        DescriptorSetLayouts _setLayouts;
        DescriptorSetLayout  _samplerLayout;
        PipelineType         _pipelineType = PipelineType::GRAPHICS;
        bool                 _localSignature = false;
    };

    class VISUALIZATION_API RootDescriptorLayout {
    protected:
        friend class Device;
        RootDescriptorLayout();

    public:
        virtual ~RootDescriptorLayout();


        RootDescriptorLayoutInit _init;
    };

    // Descriptor Object
    // the actual descriptor of a resource
    // Specify the descriptor type and the resource
    //
    struct VISUALIZATION_API DescriptorObject{
        DescriptorType  _type = DescriptorType::UNDEFINED;
        union {
            BufferPointer _buffer;
            TexturePointer _texture;
            SamplerPointer _sampler;
        };
        DescriptorObject() {}
        DescriptorObject(const DescriptorObject& src) : _type(src._type), _buffer(src._buffer) {}
        DescriptorObject& operator = (const DescriptorObject& src) { _type = (src._type); _buffer = (src._buffer); return *this; }
        DescriptorObject(DescriptorType type, BufferPointer b) : _type(type), _buffer(b) {}
        DescriptorObject(DescriptorType type, TexturePointer t) : _type(type), _texture(t) {}
        DescriptorObject(SamplerPointer s) : _type(DescriptorType::SAMPLER), _sampler(s) {}

        ~DescriptorObject() {}
    };
    using DescriptorObjects = std::vector<DescriptorObject>;

    // Descriptor Heap
    // Stores the decriptor object (aka view) on resource which are addressable by shaders
    // Register a resource in the heap to describing a view on it to get it's offset position in the heap
    // From this 
    // 
    // There is one Global Descriptor heap bound on the Device
    // 
    struct VISUALIZATION_API DescriptorHeapInit {
        int32_t _numDescritors = 1000;
        int32_t _numSamplers = 32;
    };

    class VISUALIZATION_API DescriptorHeap {
    protected:
        friend class Device;
        DescriptorHeap();

    public:
        virtual ~DescriptorHeap();

        DescriptorHeapInit _init;

        // allocate explicitely N consecutive descriptors
        // return the index of the descriptor in the heap
        // or -1 if couldn't fit
        virtual int32_t allocateDescriptors(int32_t numDescriptors) = 0;
        virtual int32_t allocateSamplers(int32_t numSamplers) = 0;
    };


    // Descriptor Set
    // reference a range of descriptors in the heap
    // and a matching descriptor set layout
    struct VISUALIZATION_API DescriptorSetInit {
        RootDescriptorLayoutPointer _rootLayout;
        int32_t _bindSetSlot = -1; // the set slot bound by this descriptor (-1 can be used for just samplers)
        bool _bindSamplers = false;
        DescriptorSetLayout _descriptorSetLayout;
    };

    class VISUALIZATION_API DescriptorSet {
    protected:
        friend class Device;
        DescriptorSet();

    public:
        virtual ~DescriptorSet();

        DescriptorSetInit _init;

        int32_t _descriptorOffset = -1; // offset of the first descriptor in the heap
        int32_t _samplerOffset = -1; // offset of the first sampler descriptor in the heap
        int32_t _numDescriptors = 0;   // numbers of descriptors

        DescriptorObjects _objects;
    };

    class VISUALIZATION_API ShaderEntry {
    protected:
        friend class Device;
    public:
        ShaderEntry();
        ShaderEntry(const ShaderEntry& src);
        virtual ~ShaderEntry();

        static const uint32_t BLOB_SIZE = 32;
        std::array<uint8_t, BLOB_SIZE> _blob;
    };

    // Shader Record
    struct VISUALIZATION_API ShaderTableInit {
        struct Record {
            ShaderEntry shaderEntry;
            DescriptorSetPointer descriptorSet;
        };

        using RecordArray = std::vector<Record>;


        RecordArray records;
    };


    class VISUALIZATION_API ShaderTable {
    protected:
        friend class Device;
        ShaderTable();

        ShaderTableInit _init;
    public:
        virtual ~ShaderTable();

    };

}
