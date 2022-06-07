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

    struct UploadSubresourceLayout {
        uint32_t subresource{ 0 };
        uint64_t byteOffset{ 0 };
        uint64_t byteLength{ 0 };
    };
    using UploadSubresourceLayoutArray = std::vector< UploadSubresourceLayout>;

    struct VISUALIZATION_API BufferInit {
        uint16_t usage{ 0 };
        uint64_t bufferSize { 0 };
        bool cpuDouble{ false };
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
        friend class Backend;
        Buffer();

        std::string _name;

        uint64_t _bufferSize;

        bool _needUpload = true;

    public:
        virtual ~Buffer();

        BufferInit _init;

        uint64_t bufferSize() const { return _bufferSize; }
        uint32_t numElements() const { return _init.numElements; }

        void* _cpuMappedAddress = nullptr;

        // true if the texture needs to be uploaded through a Batch::uploadBuffer call
        // don't forget the transitions calls!
        // this needs to be checked and done explicitely before being able to fetch from that buffer in shaders 
        bool needUpload() const { return _needUpload; }
        void touch() { _needUpload = true; }

        // Called internally 
        void notifyUploaded() { _needUpload = false; }
    };



    struct VISUALIZATION_API TextureInit {
        uint16_t usage { 0 }; // indicate the different usage expected for this resource. Default is SHADER_RESOURCE
        uint32_t width { 0 };
        uint32_t height { 0 };
        uint32_t numSlices { 0 }; // if numSlices is > 0 => array texture

        PixelFormat format { PixelFormat::R8G8B8A8_UNORM };

        std::vector<std::vector<uint8_t>> initData;
    };

    class VISUALIZATION_API Texture : public Resource {
    protected:
        // Texture is created from the device
        friend class Device;
        Texture();

        bool _needUpload = true;

    public:
        virtual ~Texture();

        TextureInit _init;

        uint32_t width() const { return _init.width; }
        uint32_t height() const { return _init.height; }
        uint32_t numSlices() const { return _init.numSlices; }
        PixelFormat format() const { return _init.format; }

        // cpuDataBuffer is allocated if required
        BufferPointer _cpuDataBuffer;

        // true if the buffer needs to be uploaded through a Batch::uploadTexture call
        // don't forget the transitions calls!
        // this needs to be checked and done explicitely before being able to fetch from that texture in shaders 
        bool needUpload() const { return _needUpload; }

        // Called internally 
        void notifyUploaded() { _needUpload = false; }

        static std::pair<UploadSubresourceLayoutArray, uint64_t> evalUploadSubresourceLayout(const TexturePointer& dest, const std::vector<uint32_t>& subresources = std::vector<uint32_t>());
    };


    struct VISUALIZATION_API BufferElementView {
        BufferPointer buffer;
        int64_t offset = 0;
        int32_t stride = 0;
    };

    struct VISUALIZATION_API GeometryInit {
        BufferElementView vertexBuffer;
        int32_t vertexCount = 0;
        PixelFormat vertexFormat = PixelFormat::R32G32B32_FLOAT;

        BufferElementView indexBuffer;
        int32_t indexCount = 0;
    };

    class VISUALIZATION_API Geometry {
    protected:
        // Geometry is created from the device
        friend class Device;
        Geometry();

        BufferPointer _tlas;

    public:
        virtual ~Geometry();
        GeometryInit _init;

        BufferPointer getTLAS() const { return _tlas; }
    };

}