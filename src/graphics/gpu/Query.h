// Query.h 
//
// Sam Gateau - October 2021
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
    
    struct QueryInit {
    };

    class Query {
    protected:
        friend class Device;
        Query();

        QueryInit _init;

    public:
        ~Query();

    };



    struct BatchTimerInit {
        int32_t numSamples = 64;
    };

    class BatchTimer {
    protected:
        friend class Device;
        BatchTimer();

        BatchTimerInit _init;

        BufferPointer _buffer; // Buffer in gpu mem used by the queris to collect result, accessible as a read_resource

        int32_t _currentSampleIndex = 0;

    public:
        ~BatchTimer();

        BufferPointer getBuffer() const { return _buffer; }

        int32_t getCurrentSampleIndex() const { return _currentSampleIndex; }
        int32_t getNumSamples() const { return _init.numSamples; }
    };
}