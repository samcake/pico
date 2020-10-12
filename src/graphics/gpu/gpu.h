// gpu.h 
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

#include <cstdint>
#include <memory>
#include <string>
#include <array>
#include "../dllmain.h" // pick dllmain relative to this file

namespace graphics {

    enum class PrimitiveTopology : uint8_t {
        POINT,
        LINE,
        TRIANGLE,
        TRIANGLE_STRIP,

        COUNT,
    };

    enum class AttribFormat : uint8_t {
        UINT32,
        VEC3,
        VEC4,
        CVEC4,

        COUNT,
    };

    enum class AttribSemantic : uint8_t {
        A = 0,
        B,
        C,

        INDEX,
        COUNT,
    };

    enum class ShaderType : uint8_t {
        PROGRAM = 0,
        VERTEX,
        PIXEL,

        COUNT,
    };

    enum class ShaderStage : uint16_t {
        VERTEX = 0x0001,
        PIXEL = 0x0100,
        ALL_GRAPHICS = 0x0101,

        COUNT = 3,
    };


    // GPU types
    class Swapchain;
    using SwapchainPointer = std::shared_ptr<Swapchain>;
    struct SwapchainInit;

    class Device;
    using DevicePointer = std::shared_ptr<Device>;
    struct DeviceInit;

    class Batch;
    using BatchPointer = std::shared_ptr<Batch>;
    struct BatchInit;

    class Buffer;
    using BufferPointer = std::shared_ptr<Buffer>;
    struct BufferInit;

    class Texture;
    using TexturePointer = std::shared_ptr<Texture>;
    struct TextureInit;

    class Sampler;
    using SamplerPointer = std::shared_ptr<Sampler>;
    struct SamplerInit;

    class Shader;
    using ShaderPointer = std::shared_ptr<Shader>;
    struct ShaderInit;
    struct ProgramInit;

    class DescriptorSetLayout;
    using DescriptorSetLayoutPointer = std::shared_ptr<DescriptorSetLayout>;
    struct DescriptorSetLayoutInit;

    class DescriptorSet;
    using DescriptorSetPointer = std::shared_ptr<DescriptorSet>;
    struct DescriptorSetInit;

    class PipelineState;
    using PipelineStatePointer = std::shared_ptr<PipelineState>;
    struct PipelineStateInit;

    class Framebuffer;
    using FramebufferPointer = std::shared_ptr<Framebuffer>;
    struct FramebufferInit;


    // Resource Types
    enum class ResourceUsage {
        INDEX_BUFFER = 0,
        VERTEX_BUFFER,
        UNIFORM_BUFFER,
        RESOURCE_BUFFER,

        COUNT,
    };

    enum class ResourceState {
        COMMON = 0,
        VERTEX_AND_CONSTANT_BUFFER,
        INDEX_BUFFER,
        RENDER_TARGET,
        UNORDERED_ACCESS,
        DEPTH_WRITE,
        DEPTH_READ,
        IMAGE_SHADER_RESOURCE,
        STREAM_OUT,
        INDIRECT_ARGUMENT,
        COPY_DEST,
        COPY_SOURCE,
        RESOLVE_DEST,
        RESOLVE_SOURCE,
        _GENERIC_READ,

        PRESENT,
        PREDICATION,


        COUNT,
    };
    enum class ResourceBarrierFlag {
        NONE = 0,
        BEGIN_ONLY,
        END_ONLY,

        COUNT,
    };

    // Descriptor types
    enum class DescriptorType : uint8_t {
        UNDEFINED = 0,
        SAMPLER,
        UNIFORM_BUFFER,            // CBV | VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        RESOURCE_BUFFER,        // SRV | VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
        STORAGE_BUFFER_UAV,        // UAV | VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
        UNIFORM_TEXEL_BUFFER_SRV,  // SRV | VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
        STORAGE_TEXEL_BUFFER_UAV,  // UAV | VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
        RESOURCE_TEXTURE,               // SRV | VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
        TEXTURE_UAV,               // UAV | VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
        PUSH_UNIFORM,             // CONSTANT | PUSH_CONSTANT

        COUNT,
    };
}

