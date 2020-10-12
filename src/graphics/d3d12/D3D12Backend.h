// D3D12Backend.h 
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

#include "gpu/Device.h"
#include "gpu/Swapchain.h"
#include "gpu/Framebuffer.h"
#include "gpu/Batch.h"
#include "gpu/Resource.h"
#include "gpu/Pipeline.h"
#include "gpu/Shader.h"
#include "gpu/Descriptor.h"

#ifdef _WINDOWS

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

namespace graphics {
    
    class D3D12Backend : public DeviceBackend {
    public:

        D3D12Backend();
        virtual ~D3D12Backend();

        static const uint8_t CHAIN_NUM_FRAMES = 3;

        // DirectX 12 Objects
        ComPtr<ID3D12Device2> _device;
        ComPtr<ID3D12CommandQueue> _commandQueue;

        // Functions points for functions that need to be loaded
        PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER           fnD3D12CreateRootSignatureDeserializer = nullptr;
        PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE           fnD3D12SerializeVersionedRootSignature = nullptr;
        PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER fnD3D12CreateVersionedRootSignatureDeserializer = nullptr;

        // Synchronization objects
        ComPtr<ID3D12Fence> _fence;
        uint64_t _fenceValue = 0;
        uint64_t _frameFenceValues[D3D12Backend::CHAIN_NUM_FRAMES] = {};
        HANDLE _fenceEvent;

        SwapchainPointer createSwapchain(const SwapchainInit& init) override;
        void resizeSwapchain(const SwapchainPointer& swapchain, uint32_t width, uint32_t height) override;

        FramebufferPointer createFramebuffer(const FramebufferInit& init) override;

        BatchPointer createBatch(const BatchInit& init) override;

        BufferPointer createBuffer(const BufferInit& init) override;

        TexturePointer createTexture(const TextureInit& init) override;

        ShaderPointer createShader(const ShaderInit& init) override;
        ShaderPointer createProgram(const ProgramInit& init) override;

        SamplerPointer createSampler(const SamplerInit& init) override;

        PipelineStatePointer createPipelineState(const PipelineStateInit& init) override;
   
        DescriptorSetLayoutPointer createDescriptorSetLayout(const DescriptorSetLayoutInit& init) override;
        DescriptorSetPointer createDescriptorSet(const DescriptorSetInit& init) override;
        void updateDescriptorSet(DescriptorSetPointer& descriptorSet, DescriptorObjects& objects) override;

        void executeBatch(const BatchPointer& batch) override;
        void presentSwapchain(const SwapchainPointer& swapchain) override;

        void flush();
    };

    class D3D12SwapchainBackend : public Swapchain {
    public:
        friend class D3D12Backend;
        D3D12SwapchainBackend();
        virtual ~D3D12SwapchainBackend();

        ComPtr<IDXGISwapChain4> _swapchain;
        ComPtr<ID3D12Resource> _backBuffers[D3D12Backend::CHAIN_NUM_FRAMES];
        ComPtr<ID3D12DescriptorHeap> _rtvDescriptorHeap;
        UINT _rtvDescriptorSize;

        ComPtr<ID3D12Resource> _depthBuffer;
        ComPtr<ID3D12DescriptorHeap> _dsvDescriptorHeap;
        UINT _dsvDescriptorSize { 0 };
    };

    class D3D12FramebufferBackend : public Framebuffer {
    public:
        friend class D3D12Backend;
        D3D12FramebufferBackend();
        virtual ~D3D12FramebufferBackend();

    };

    class D3D12BatchBackend : public Batch {
    public:
        friend class D3D12Backend;
        D3D12BatchBackend();
        virtual ~D3D12BatchBackend();

        void begin(uint8_t currentIndex) override;
        void end() override;

        void beginPass(const SwapchainPointer& swapchain, uint8_t currentIndex) override;
        void endPass() override;

        void clear(const SwapchainPointer& swapchain, uint8_t index, const core::vec4& color, float depth) override;
        void resourceBarrierTransition(
            ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const SwapchainPointer& swapchain, uint8_t currentIndex, uint32_t subresource) override;
        void resourceBarrierTransition(
            ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const BufferPointer& buffer) override;
        void resourceBarrierTransition(
            ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const TexturePointer& buffer, uint32_t subresource) override;

        void setViewport(const core::vec4& viewport) override;
        void setScissor(const core::vec4& scissor) override;

        void setPipeline(const PipelineStatePointer& pipeline) override;
        void bindDescriptorSet(const DescriptorSetPointer& descriptorSet) override;
        void bindPushUniform(uint32_t slot, uint32_t size, const uint8_t* data) override;

        void bindIndexBuffer(const BufferPointer& buffer) override;
        void bindVertexBuffers(uint32_t num, const BufferPointer* buffers) override;

        void draw(uint32_t numPrimitives, uint32_t startIndex) override;
        void drawIndexed(uint32_t numPrimitives, uint32_t startIndex) override;

        void uploadTexture(const TexturePointer& dest, const BufferPointer& src) override;

        ComPtr<ID3D12GraphicsCommandList> _commandList;
        ComPtr<ID3D12CommandAllocator> _commandAllocators[D3D12Backend::CHAIN_NUM_FRAMES];

        UINT _currentBackBufferIndex { 0 };

        static const D3D12_RESOURCE_STATES ResourceStates[uint32_t(ResourceState::COUNT)];
        static const D3D12_RESOURCE_BARRIER_FLAGS  ResourceBarrieFlags[uint32_t(ResourceBarrierFlag::COUNT)];

        static const D3D12_PRIMITIVE_TOPOLOGY_TYPE  PrimitiveTopologyTypes[uint32_t(PrimitiveTopology::COUNT)];
        static const D3D12_PRIMITIVE_TOPOLOGY  PrimitiveTopologies[uint32_t(PrimitiveTopology::COUNT)];

    };

    class D3D12BufferBackend : public Buffer {
    public:
        friend class D3D12Backend;
        D3D12BufferBackend();
        virtual ~D3D12BufferBackend();

        ComPtr<ID3D12Resource> _resource;


        D3D12_CONSTANT_BUFFER_VIEW_DESC _uniformBufferView;
        D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;
        D3D12_INDEX_BUFFER_VIEW _indexBufferView;
        D3D12_SHADER_RESOURCE_VIEW_DESC _resourceBufferView;
    };

    class D3D12TextureBackend : public Texture {
    public:
        friend class D3D12Backend;
        D3D12TextureBackend();
        virtual ~D3D12TextureBackend();

        ComPtr<ID3D12Resource> _resource;

        D3D12_SHADER_RESOURCE_VIEW_DESC   _shaderResourceViewDesc;
   
    };

    class D3D12ShaderBackend : public Shader {
    public:
        friend class D3D12Backend;
        D3D12ShaderBackend();
        virtual ~D3D12ShaderBackend();

        ComPtr<ID3DBlob> _shaderBlob;

        static const std::string ShaderTypes[uint32_t(ShaderType::COUNT)];
    };

    class D3D12SamplerBackend : public Sampler {
    public:
        friend class D3D12Backend;
        D3D12SamplerBackend();
        virtual ~D3D12SamplerBackend();

        D3D12_SAMPLER_DESC _samplerDesc;
    };

    class D3D12PipelineStateBackend : public PipelineState {
    public:
        friend class D3D12Backend;
        D3D12PipelineStateBackend();
        virtual ~D3D12PipelineStateBackend();

        ComPtr<ID3D12RootSignature> _rootSignature;
        ComPtr<ID3D12PipelineState> _pipelineState;
        D3D12_PRIMITIVE_TOPOLOGY _primitive_topology;
    };

    class D3D12DescriptorSetLayoutBackend : public DescriptorSetLayout {
    public:
        friend class D3D12Backend;
        D3D12DescriptorSetLayoutBackend();
        virtual ~D3D12DescriptorSetLayoutBackend();

        std::vector< uint32_t > _dxParamIndices;

        ComPtr<ID3D12RootSignature> _rootSignature;
        uint32_t cbvsrvuav_count = 0;
        uint32_t sampler_count = 0;
        uint32_t push_count = 0;
    };

    class D3D12DescriptorSetBackend : public DescriptorSet {
    public:
        friend class D3D12Backend;
        D3D12DescriptorSetBackend();
        virtual ~D3D12DescriptorSetBackend();

        ComPtr < ID3D12DescriptorHeap> _cbvsrvuav_heap;
        ComPtr < ID3D12DescriptorHeap> _sampler_heap;
        std::vector< uint32_t > _dxHeapOffsets;
        //std::vector< D3D12_CPU_DESCRIPTOR_HANDLE > _dxCPUHandles;
        std::vector< D3D12_GPU_DESCRIPTOR_HANDLE > _dxGPUHandles;
        std::vector< uint32_t > _dxRootParameterIndices;
        uint32_t cbvsrvuav_count = 0;
        uint32_t sampler_count = 0;
        uint32_t push_count = 0;
    };

}

#endif _WINDOWS
