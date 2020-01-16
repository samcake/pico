// Mesh.h 
//
// Sam Gateau - 2020/1/1
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

#include "../Forward.h"

#include "../mas.h"

#include <vector>

namespace poco {

    enum class AttribFormat : uint8_t {
        UINT32,
        VEC3,
        CVEC4,

        COUNT,
    };

    enum class Topology : uint8_t {
        POINT,
        LINE,
        TRIANGLE,
        TRIANGLE_STRIP,

        COUNT,
    };

    struct AttribAccessor {
        AttribFormat _format{ AttribFormat::UINT32 };
        uint8_t _bufferIndex{ 0 };
        uint16_t _streamOffset{ 0 };
    };

    struct AttribBufferView {
        uint8_t _bufferIndex{ 0 };
        uint16_t _byteStride{ 0 };
        uint32_t _byteOffset{ 0 };
        uint32_t _byteLength{ 0xFFFFFFFF };
    };

    class StreamAccessor {
    public:
        virtual ~StreamAccessor() {};
 
        virtual uint8_t getNumAttribs() const = 0;
        virtual uint8_t getNumBuffers() const = 0;

        virtual const AttribAccessor* getAttrib(uint8_t a) const = 0;
        virtual const AttribBufferView* getBuffer(uint8_t b) const = 0;
    };

    template <int A, int B> class StreamAccessorInstanced : public StreamAccessor {
    public:
        StreamAccessorInstanced() {}
        virtual ~StreamAccessorInstanced() {}

        uint8_t getNumAttribs() const override { return A; }
        uint8_t getNumBuffers() const override { return B; }
        const AttribAccessor* getAttrib(uint8_t a) const override { return attribs + a; }
        const AttribBufferView* getBuffer(uint8_t b) const override { return bufferViews + b; }

        AttribAccessor attribs[A];
        AttribBufferView bufferViews[B];
    };

    using IndexStreamAccessor = StreamAccessorInstanced<1, 1>;

    struct AttribBuffer {
        std::vector<uint8_t> _data;

        AttribBuffer() {}
        AttribBuffer(void* data, size_t size) : _data(size, 0) { memcpy(_data.data(), data, size); }
    };
    using AttribBufferPointer = std::shared_ptr<AttribBuffer>;
    using AttribBufferPointers = std::vector<AttribBufferPointer>;

    struct StreamView {
        StreamAccessor* _accessor;
        AttribBufferPointers _buffers;
    };
    
    class Mesh {
    public:
        Mesh();
        ~Mesh();

        StreamView _vertexBuffers;

        StreamView _indexBuffer { new IndexStreamAccessor(), { AttribBufferPointer() } };

        Topology _topology { Topology::POINT };
    };

}