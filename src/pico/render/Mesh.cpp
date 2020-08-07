// Mesh.cpp
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
#include "Mesh.h"
#include <algorithm>

namespace pico
{

    Mesh::Mesh()
    {

    }

    Mesh::~Mesh() {

    }

    void Mesh::evalMinMaxMidPos() {
        auto numVertices = _vertexStream.getNumElements();
        picoAssert((numVertices > 0));

        uint32_t posStride = 0;
        auto posBufferBegin = getPositionBegin(posStride);
        picoAssert(posBufferBegin != nullptr);

        auto positionBegin = reinterpret_cast<const core::vec3*> (posBufferBegin);
        _bounds._minPos = _bounds._maxPos = _bounds._midPos = (*positionBegin);

        for (uint32_t i = 1; i < numVertices; ++i) {

            auto bufferOffset = posBufferBegin + (i * posStride);
            auto position = reinterpret_cast<const core::vec3*> (bufferOffset);
            _bounds._minPos.x = std::min(position->x, _bounds._minPos.x);
            _bounds._minPos.y = std::min(position->y, _bounds._minPos.y);
            _bounds._minPos.z = std::min(position->z, _bounds._minPos.z);
            _bounds._maxPos.x = std::max(position->x, _bounds._maxPos.x);
            _bounds._maxPos.y = std::max(position->y, _bounds._maxPos.y);
            _bounds._maxPos.z = std::max(position->z, _bounds._maxPos.z);
            _bounds._midPos = _bounds._midPos + (*position);
        }

        _bounds._midPos = _bounds._midPos * (1.0f / (float)numVertices);
    }


    MeshPointer Mesh::createFromPointArray(const StreamLayout& layout, uint32_t numVertices, const uint8_t* points) {
        auto mesh = std::make_shared<Mesh>();
        mesh->_vertexStream._streamLayout = layout;

        AttribBufferPointer vertices;
        size_t verticesByteSize = numVertices * layout.streamAttribSize(0);

        vertices = std::make_shared<AttribBuffer>((void*)points, verticesByteSize);
        mesh->_vertexStream._buffers.push_back(vertices);
        mesh->evalMinMaxMidPos();

        return mesh;
    }

    MeshPointer Mesh::createFromIndexedTriangleArray(const StreamLayout& layout, uint32_t numVertices, const uint8_t* points, uint32_t numIndices, const uint32_t* indices) {
        auto mesh = std::make_shared<Mesh>();

        // Adjust vertex StreamLayout bufferView[0] with the incoming packing of vertices and indices in the same buffer
        mesh->_vertexStream._streamLayout = layout;
        uint32_t verticesByteSize = numVertices * layout.streamAttribSize(0);
        uint32_t indicesByteSize = numIndices * sizeof(uint32_t);
        mesh->_vertexStream._streamLayout.editBufferView(0)->_byteLength = verticesByteSize;

        // Define indexStream layout to be found after vertices in the same buffer
        AttribArray<1> indexAttrib{ {{ AttribSemantic::INDEX, AttribFormat::UINT32, 0 }} };
        AttribBufferViewArray<1> indexBufferViews{ {{0, indicesByteSize, 4}} };
        mesh->_indexStream._streamLayout = StreamLayout::build(indexAttrib, indexBufferViews);

        // Pack vertices then indices in the same buffer
        AttribBufferPointer vbuffer = std::make_shared<AttribBuffer>((void*)points, (size_t)(verticesByteSize));
        AttribBufferPointer ibuffer = std::make_shared<AttribBuffer>((void*)indices, (size_t)(indicesByteSize));

        // and assign it in both streams as the buffer 0
        mesh->_vertexStream._buffers.push_back(vbuffer);
        mesh->_indexStream._buffers[0] = (ibuffer);

        mesh->evalMinMaxMidPos();

        return mesh;
    }
} // !namespace pico