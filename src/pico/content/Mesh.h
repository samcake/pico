// Mesh.h 
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

#include "../Forward.h"
#include "../Api.h"

#include "../mas.h"

#include "../gpu/gpu.h"

#include <vector>

namespace pico {

    struct AttribBuffer {
        std::vector<uint8_t> _data;

        uint32_t getSize() const { return _data.size(); }

        AttribBuffer() {}
        AttribBuffer(void* data, size_t size) : _data(size, 0) { if (data) memcpy(_data.data(), data, size); }
    };
    using AttribBufferPointer = std::shared_ptr<AttribBuffer>;
    using AttribBufferPointers = std::vector<AttribBufferPointer>;

    struct StreamView {
        StreamLayout _streamLayout;
        AttribBufferPointers _buffers;

        uint32_t getNumElements() const {
            auto buffer0Size = _buffers[0]->_data.size();
            auto buffer0ByteLength = _streamLayout.evalBufferViewByteLength(0, buffer0Size);
            
            auto elementStride = _streamLayout.evalBufferViewByteStride(0);

            auto numElements = buffer0ByteLength / elementStride;
            return numElements;
        }

        const uint8_t* getBufferBegin(AttribSemantic semantic, uint32_t& stride) const {
            auto attribIndex = _streamLayout.findAttribAt(semantic);
            if (attribIndex != StreamLayout::INVALID_ATTRIB_INDEX) {
                auto attrib = _streamLayout.getAttrib(attribIndex);
                auto offset = _streamLayout.evalBufferViewByteOffsetForAttribute(attribIndex);
                stride = _streamLayout.evalBufferViewByteStride(attribIndex);
                return _buffers[attrib->_bufferIndex]->_data.data() + offset;
            }
            return nullptr;
        }
    };
    
    class Mesh {
    public:
        Mesh();
        ~Mesh();

        StreamView _vertexBuffers;
        uint32_t getNumVertices() const { return _vertexBuffers.getNumElements(); }
        const uint8_t* getPositionBegin(uint32_t& stride) const { return (_vertexBuffers.getBufferBegin(AttribSemantic::A, stride)); }

        StreamView _indexBuffer { StreamLayout::build( Attribs<1>(), AttribBufferViews<1>() ), { AttribBufferPointer() } };

        Topology _topology { Topology::POINT };

        vec3 _minPos;
        vec3 _maxPos;
        vec3 _midPos;
        void evalMinMaxMidPos();
    };

}