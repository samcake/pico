// Forward.h
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
#include <iostream>
#include "dllmain.h"

namespace pico {

    class Window;
    using WindowPointer = std::shared_ptr<Window>;
    struct WindowInit;

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

    // Render types
    class Scene;
    using ScenePointer = std::shared_ptr<Scene>;

    class Camera;
    using CameraPointer = std::shared_ptr<Camera>;

    class Renderer;
    using RendererPointer = std::shared_ptr<Renderer>;

    class Viewport;
    using ViewportPointer = std::shared_ptr<Viewport>;

    // Content types
    class Mesh;
    using MeshPointer = std::shared_ptr<Mesh>;

    class PointCloud;
    using PointCloudPointer = std::shared_ptr<PointCloud>;

}
#define picoLog() ::pico::api::log(__FILE__, __LINE__, __FUNCTION__)
#define picoAssert(t) ::pico::api::_assert((t), __FILE__, __LINE__, __FUNCTION__)

