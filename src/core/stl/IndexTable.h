// IndexTable.h 
//
// Sam Gateau - October 2020
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
#include <stdint.h>
#include <vector>
#include <atomic>
#include <mutex>

//#include "dllmain.h"

namespace core {

    class IndexTable {
    public:
        using Index = uint32_t;
        using Indices = std::vector<Index>;
        static const Index INVALID_INDEX{ (Index) -1 };

        IndexTable(Index capacity = (Index)-1) : _capacity(capacity) {}
        ~IndexTable() {}

        Index getCapacity() const { return _capacity; }
        Index getNumValidElements() const { return _num_allocated_elements - (Index)_invalid_elements.size(); }
        Index getNumAllocatedElements() const { return _num_allocated_elements; }

        bool isValid(Index index) const {
            if (index < 0 || index >= _num_allocated_elements) {
                return false;
            }
            for (const auto& invalid : _invalid_elements) {
                if (index == invalid) {
                    return false;
                }
            }
            return true;
        }

        auto allocate() {
            struct result { Index index; bool recycle; };
            Index new_index;
            if (_invalid_elements.size()) {
                new_index = _invalid_elements.back();
                _invalid_elements.pop_back();
                return result{ new_index, true};
            } else {
                new_index = _num_allocated_elements;
                _num_allocated_elements++;
                return result{ new_index, false};
            }
        }

        Indices allocate(Index num_elements) {
            Indices allocated(num_elements, INVALID_INDEX);
            for (auto& index : allocated) {
                index = allocate().index;
            }
            return std::move(allocated);
        }

        Index allocateContiguous(Index num_elements) {
            if (num_elements) {
                Index new_index = _num_allocated_elements;
                if (new_index + num_elements < getCapacity()) {
                    _num_allocated_elements += num_elements;
                    return new_index;
                }
            }
            return INVALID_INDEX;
        }

        void free(Index index) {
            _invalid_elements.push_back(index);
        }

        void free(const Indices& indices) {
            _invalid_elements.insert(_invalid_elements.end(), indices.begin(), indices.end());
        }

    private:
        Index _num_allocated_elements{ (Index) 0 };
        std::vector<Index> _invalid_elements;
        const Index _capacity{ (Index) -1 };
    };

}
