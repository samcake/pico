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
#include <vector>
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

    enum class PixelFormat : uint8_t {
        UNKNOWN,
        R32G32B32A32_TYPELESS,
        R32G32B32A32_FLOAT,
        R32G32B32A32_UINT,
        R32G32B32A32_SINT,
        R32G32B32_TYPELESS,
        R32G32B32_FLOAT,
        R32G32B32_UINT,
        R32G32B32_SINT,
        R16G16B16A16_TYPELESS,
        R16G16B16A16_FLOAT,
        R16G16B16A16_UNORM,
        R16G16B16A16_UINT,
        R16G16B16A16_SNORM,
        R16G16B16A16_SINT,
        R32G32_TYPELESS,
        R32G32_FLOAT,
        R32G32_UINT,
        R32G32_SINT,
        R32G8X24_TYPELESS,
        D32_FLOAT_S8X24_UINT,
        R32_FLOAT_X8X24_TYPELESS,
        X32_TYPELESS_G8X24_UINT,
        R10G10B10A2_TYPELESS,
        R10G10B10A2_UNORM,
        R10G10B10A2_UINT,
        R11G11B10_FLOAT,
        R8G8B8A8_TYPELESS,
        R8G8B8A8_UNORM,
        R8G8B8A8_UNORM_SRGB,
        R8G8B8A8_UINT,
        R8G8B8A8_SNORM,
        R8G8B8A8_SINT,
        R16G16_TYPELESS,
        R16G16_FLOAT,
        R16G16_UNORM,
        R16G16_UINT,
        R16G16_SNORM,
        R16G16_SINT,
        R32_TYPELESS,
        D32_FLOAT,
        R32_FLOAT,
        R32_UINT,
        R32_SINT,
        R24G8_TYPELESS,
        D24_UNORM_S8_UINT,
        R24_UNORM_X8_TYPELESS,
        X24_TYPELESS_G8_UINT,
        R8G8_TYPELESS,
        R8G8_UNORM,
        R8G8_UINT,
        R8G8_SNORM,
        R8G8_SINT,
        R16_TYPELESS,
        R16_FLOAT,
        D16_UNORM,
        R16_UNORM,
        R16_UINT,
        R16_SNORM,
        R16_SINT,
        R8_TYPELESS,
        R8_UNORM,
        R8_UINT,
        R8_SNORM,
        R8_SINT,
        
        COUNT,
    };

    enum class ShaderType : uint8_t {
        PROGRAM = 0,
        VERTEX,
        PIXEL,
    
        COMPUTE,

        COUNT,
    };

    enum class ShaderStage : uint16_t {
        VERTEX = 0x0001,
        PIXEL = 0x0100,
        ALL_GRAPHICS = 0x0101,

        COMPUTE = 0x1000,

        COUNT = 4,
    };

    enum class PipelineType : uint8_t {
        GRAPHICS = 0,
        COMPUTE,

        COUNT,
    };

    // GPU types
    class Swapchain;
    using SwapchainPointer = std::shared_ptr<Swapchain>;
    struct SwapchainInit;

    class Device;
    using DevicePointer = std::shared_ptr<Device>;
    using DeviceWeakPtr = std::weak_ptr<Device>;
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
    struct UploadSubresourceLayout;
    using UploadSubresourceLayoutArray = std::vector<UploadSubresourceLayout>;

    class Sampler;
    using SamplerPointer = std::shared_ptr<Sampler>;
    struct SamplerInit;

    class Shader;
    using ShaderPointer = std::shared_ptr<Shader>;
    using ShaderWeakPtr = std::weak_ptr<Shader>;
    struct ShaderInit;
    struct ProgramInit;

    class RootDescriptorLayout;
    using RootDescriptorLayoutPointer = std::shared_ptr<RootDescriptorLayout>;
    struct RootDescriptorLayoutInit;

    class DescriptorHeap;
    using DescriptorHeapPointer = std::shared_ptr<DescriptorHeap>;
    struct DescriptorHeapInit;

    class DescriptorSet;
    using DescriptorSetPointer = std::shared_ptr<DescriptorSet>;
    struct DescriptorSetInit;

    class PipelineState;
    using PipelineStatePointer = std::shared_ptr<PipelineState>;
    using PipelineStateWeakPtr = std::weak_ptr<PipelineState>;
    struct GraphicsPipelineStateInit;
    struct ComputePipelineStateInit;

    class Framebuffer;
    using FramebufferPointer = std::shared_ptr<Framebuffer>;
    struct FramebufferInit;


    class BatchTimer;
    using BatchTimerPointer = std::shared_ptr<BatchTimer>;
    struct BatchTimerInit;

    // Resource Types
    enum ResourceUsage : uint16_t {
        INDEX_BUFFER = 0x0001,
        VERTEX_BUFFER = 0x0002,
        UNIFORM_BUFFER = 0x0004,
        RESOURCE_BUFFER = 0x0008,
        RW_RESOURCE_BUFFER = 0x0010,
        RESOURCE_TEXTURE = 0x0020,
        RW_RESOURCE_TEXTURE = 0x0040,
        RENDER_TARGET = 0x0080,
        GENERIC_READ_BUFFER = 0x0100,

        COUNT = 9,
    };

    enum class ResourceState {
        COMMON = 0,
        VERTEX_AND_CONSTANT_BUFFER,
        INDEX_BUFFER,
        RENDER_TARGET,
        UNORDERED_ACCESS,
        DEPTH_WRITE,
        DEPTH_READ,
        SHADER_RESOURCE,
        STREAM_OUT,
        INDIRECT_ARGUMENT,
        COPY_DEST,
        COPY_SOURCE,
        RESOLVE_DEST,
        RESOLVE_SOURCE,
        GENERIC_READ_BUFFER,

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
        RW_RESOURCE_BUFFER,        // UAV | VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
        RESOURCE_TEXTURE,               // SRV | VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
        RW_RESOURCE_TEXTURE,               // UAV | VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
        PUSH_UNIFORM,             // CONSTANT | PUSH_CONSTANT

        COUNT,
    };
}

