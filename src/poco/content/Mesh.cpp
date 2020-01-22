// Mesh.cpp
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
#include "Mesh.h"
#include "../Api.h"

#include <algorithm>

using namespace poco;

Mesh::Mesh() {

}

Mesh::~Mesh() {

}

void Mesh::evalMinMaxPos() {
    auto numVertices = _vertexBuffers.getNumElements();
    pocoAssert((numVertices > 0));

    uint16_t posStride = 0;
    auto posBufferBegin = getPositionBegin(posStride);
    pocoAssert(posBufferBegin != nullptr);

    auto positionBegin = reinterpret_cast<const vec3*> (posBufferBegin);
    _minPos = _maxPos = (*positionBegin);

    for (uint32_t i = 1; i < numVertices; ++i) {

        auto bufferOffset = posBufferBegin + (size_t) (i * posStride);
        auto position = reinterpret_cast<const vec3*> (bufferOffset);
        _minPos.x = std::min(position->x, _minPos.x);
        _minPos.y = std::min(position->y, _minPos.y);
        _minPos.z = std::min(position->z, _minPos.z);
        _maxPos.x = std::max(position->x, _maxPos.x);
        _maxPos.y = std::max(position->y, _maxPos.y);
        _maxPos.z = std::max(position->z, _maxPos.z);
    }

}
