// Resource.h 
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

    class VISUALIZATION_API Resource {
    public:
        virtual ~Resource();
    protected:
        Resource();

    };


    struct VISUALIZATION_API BufferInit {
        uint8_t usage;
        uint64_t bufferSize { 0 };
        bool hostVisible {false};
        bool swapchainable {false};
        bool raw {false};

        // VertexBuffer
        uint32_t vertexStride { 0 };

        // ResourceBuffer
        uint32_t firstElement{ 0 };
        uint32_t numElements{ 0 };
        uint32_t structStride{ 0 };
    };

    class VISUALIZATION_API Buffer : public Resource {
    protected:
        // Buffer is created from the device
        friend class Device;
        Buffer();

    public:
        virtual ~Buffer();

        uint32_t getNumElements() const { return _init.numElements; }

        BufferInit _init;
        void* _cpuMappedAddress = nullptr;
        uint64_t _bufferSize;
    };



    struct VISUALIZATION_API TextureInit {
        uint32_t width { 0 };
        uint32_t height { 0 };

        std::vector<uint8_t> initData;
    };

    class VISUALIZATION_API Texture : public Resource {
    protected:
        // Texture is created from the device
        friend class Device;
        Texture();

    public:
        virtual ~Texture();

        TextureInit _init;
        void* _cpuMappedAddress = nullptr;
        uint64_t _bufferSize;
    };
}