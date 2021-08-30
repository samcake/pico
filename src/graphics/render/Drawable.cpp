// Drawable.cpp
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
#include "Drawable.h"
#include "gpu/Resource.h"
#include "gpu/Device.h"

using namespace graphics;


Drawable Drawable::null;

DrawableID DrawableStore::newID() {
    return _indexTable.allocate();
}

Drawable DrawableStore::allocate(Drawable drawable) {

    DrawableID new_id = drawable.id();
    bool allocated = new_id == (_indexTable.getNumAllocatedElements() - 1);

    auto bound = drawable.getBound();

    if (allocated) {
        _drawables.push_back(drawable);
        _bounds.push_back({ bound });
    }
    else {
        _drawables[new_id] = (drawable);
        _bounds[new_id] = { bound };
    }

    if (new_id < _num_buffers_elements) {
        reinterpret_cast<GPUDrawableBound*>(_drawables_buffer->_cpuMappedAddress)[new_id]._local_box = (bound);
    }

    return drawable;
}

void DrawableStore::free(DrawableID index) {
    if (_indexTable.isValid(index)) {
        _indexTable.free(index);

        _drawables[index] = Drawable::null;
        _bounds[index] = { };

        if (index < _num_buffers_elements) {
            reinterpret_cast<GPUDrawableBound*>(_drawables_buffer->_cpuMappedAddress)[index]._local_box = core::aabox3();
        }
    }
}

int32_t DrawableStore::reference(DrawableID index) {
    if (_indexTable.isValid(index)) {
        return _drawables[index].reference();
    } else { 
        return 0;
    }
}

int32_t DrawableStore::release(DrawableID index) {
    if (_indexTable.isValid(index)) {
        return _drawables[index].release();
    }
    else {
        return 0;
    }
}

void DrawableStore::resizeBuffers(const DevicePointer& device, uint32_t numElements) {

    if (_num_buffers_elements < numElements) {
        auto capacity = (numElements);

        graphics::BufferInit drawables_buffer_init{};
        drawables_buffer_init.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
        drawables_buffer_init.hostVisible = true;
        drawables_buffer_init.bufferSize = capacity * sizeof(GPUDrawableBound);
        drawables_buffer_init.firstElement = 0;
        drawables_buffer_init.numElements = capacity;
        drawables_buffer_init.structStride = sizeof(GPUDrawableBound);

        _drawables_buffer = device->createBuffer(drawables_buffer_init);
        
        _num_buffers_elements = capacity;
    }
}
