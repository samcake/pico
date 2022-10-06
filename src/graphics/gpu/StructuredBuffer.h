// StructuredBuffer.h 
//
// Sam Gateau - September 2022
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

#include <mutex>
#include "Resource.h"
#include "Batch.h"

namespace graphics {

    // A StructuredBuffer is a managed dual cpu buffer / gpu buffer array of contiguous T elements
    // The array (or buffer) is managing the storage of the elements.
    // An element can be embedded in an individual instance by using the StructuredBuffer<T>::Handle struct
    template <typename T>
    struct StructuredBuffer {
        using Index = uint32_t;
        using IndexArray = std::vector<Index>;
        using Array = std::vector<T>;

    private:
        mutable std::mutex  _cpu_access;

        std::atomic_uint32_t _cpu_version{ 0xFFFFFFFF };
        Array               _cpu_array;
        Index               _cpu_capacity{ 0 };

        uint32_t            _gpu_version{ 0xFFFFFFFF };
        BufferPointer       _gpu_buffer;
        Index               _gpu_capacity{ 0 };

    public:

        constexpr size_t sizeOfElement() const { return sizeof(T); };

        inline const T* unsafe_data(uint32_t index) const {
            return _cpu_array.data() + index;
        }
        inline T* unsafe_data(uint32_t index) {
            return _cpu_array.data() + index;
        }

        inline void allocate_element(Index index, const T* init) {
            const std::lock_guard<std::mutex> access_lock(_cpu_access);
            ++_cpu_version;
            if (index < _cpu_array.size()) {
                _cpu_array[index] = (init ? *init : T());
            } else {
                _cpu_array.emplace_back((init ? *init : T()));
                _cpu_capacity = _cpu_array.capacity();
            }
        }

        inline void set_element(Index index, const T* val) {
            const std::lock_guard<std::mutex> access_lock(_cpu_access);
            ++_cpu_version;      
            _cpu_array[index] = (val ? *val : T());
        }

        inline void reserve(const DevicePointer& device, Index capacity) {
            // cpu array yeah
            if (_cpu_capacity < capacity) {
                const std::lock_guard<std::mutex> access_lock(_cpu_access);
                _cpu_array.reserve(capacity);
                _cpu_capacity = _cpu_array.capacity();
                // cpu_version hasn't changed, values are the same
            }

            if (_gpu_capacity < capacity) {
                const std::lock_guard<std::mutex> access_lock(_cpu_access);
                _gpu_version = 0xFFFFFFFF; // reset gpu version to do a full recopy on next sync

                graphics::BufferInit items_buffer_init{};
                items_buffer_init.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
                items_buffer_init.hostVisible = true;
                items_buffer_init.bufferSize = capacity * sizeof(T);
                items_buffer_init.firstElement = 0;
                items_buffer_init.numElements = capacity;
                items_buffer_init.structStride = sizeof(T);
                items_buffer_init.cpuDouble = true;
                _gpu_buffer = device->createBuffer(items_buffer_init);

                _gpu_capacity = capacity;
            }
        }

        inline void sync_gpu_from_cpu(const BatchPointer& batch) {
            // Capture the cpu version right now
            auto cpu_version = _cpu_version.load();

            // Schedule copy data from cpu to gpu here if versions are different
            if (_gpu_version != cpu_version) {             
                const std::lock_guard<std::mutex> cpulock(_cpu_access);

                _gpu_version = cpu_version;

                memcpy(_gpu_buffer->_cpuMappedAddress, _cpu_array.data(), _cpu_array.size() * sizeOfElement());
                
                batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::SHADER_RESOURCE, graphics::ResourceState::COPY_DEST, _gpu_buffer);
                batch->uploadBuffer(_gpu_buffer);
                batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::COPY_DEST, graphics::ResourceState::SHADER_RESOURCE, _gpu_buffer);
            }
        }

        inline void sync_gpu_from_cpu(const BatchPointer& batch, const IndexArray& touchedElementsOrdered) {
            // Capture the cpu version right now
            auto cpu_version = _cpu_version.load();

            // Schedule copy data from cpu to gpu here if versions are different
            if (touchedElementsOrdered.size() || _gpu_version != cpu_version) {
                const std::lock_guard<std::mutex> cpulock(_cpu_access);

                _gpu_version = cpu_version;

                // only copy the touched values              
                for (auto index : touchedElementsOrdered) {
                    auto gpu_p = reinterpret_cast<T*>(_gpu_buffer->_cpuMappedAddress) + index;
                    auto cpu_p = _cpu_array.data() + index;
                    memcpy(gpu_p, (void*)cpu_p, sizeOfElement());
                }

                batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::SHADER_RESOURCE, graphics::ResourceState::COPY_DEST, _gpu_buffer);
                batch->uploadBuffer(_gpu_buffer);
                batch->resourceBarrierTransition(graphics::ResourceBarrierFlag::NONE, graphics::ResourceState::COPY_DEST, graphics::ResourceState::SHADER_RESOURCE, _gpu_buffer);
            }
        }

        inline BufferPointer gpu_buffer() const { return _gpu_buffer; }

        using ReadLock = std::pair< const T*, std::lock_guard<std::mutex>>;
        using WriteLock = std::pair< T*, std::lock_guard<std::mutex>>;

        inline ReadLock read(Index index) const {
            return  ReadLock(unsafe_data(index), _cpu_access);
        }
        inline WriteLock write(Index index) {
            ++_cpu_version; // Write access increment cpu version
            return  WriteLock(unsafe_data(index), _cpu_access);
        }

        struct Handle {
            StructuredBuffer<T>* _buffer;
            Index            _index{ 0xFFFFFFFF };

            inline ReadLock read() const {
                return  _buffer->read(_index);
            }
            inline WriteLock write() const {
                return  _buffer->write(_index);
            }
        };
    };
}
