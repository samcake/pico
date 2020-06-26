// Descriptor.h 
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

namespace pico {

    struct VISUALIZATION_API DescriptorLayout {
        DescriptorType  _type;
        ShaderStage     _shaderStage;
        uint32_t        _binding;
        uint32_t        _count;
    };

    using DescriptorLayouts = std::vector<DescriptorLayout>;



    struct VISUALIZATION_API DescriptorSetLayoutInit {
        DescriptorLayouts _layouts;
    };

    class VISUALIZATION_API DescriptorSetLayout {
    protected:
        friend class Device;
        DescriptorSetLayout();

    public:
        virtual ~DescriptorSetLayout();

        DescriptorSetLayoutInit _init;
    };

    struct VISUALIZATION_API DescriptorObject{
        std::vector<BufferPointer> _uniformBuffers;
        std::vector<BufferPointer> _buffers;
        std::vector<TexturePointer> _textures;
        std::vector<SamplerPointer> _samplers;
    };
    using DescriptorObjects = std::vector<DescriptorObject>;


    struct VISUALIZATION_API DescriptorSetInit {
        DescriptorSetLayoutPointer _layout;
    };

    class VISUALIZATION_API DescriptorSet {
    protected:
        // Buffer is created from the device
        friend class Device;
        DescriptorSet();

    public:
        virtual ~DescriptorSet();

        DescriptorSetInit _init;

        DescriptorObjects _objects;
    };
}
