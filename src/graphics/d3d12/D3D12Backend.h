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
#include "gpu/Query.h"

#include <core/stl/IndexTable.h>

#ifdef _WINDOWS

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <dxcapi.h>

namespace graphics {
    class D3D12DescriptorHeapBackend;

    class D3D12DescriptorHeapBackend : public DescriptorHeap {
    public:
        D3D12DescriptorHeapBackend();
        virtual ~D3D12DescriptorHeapBackend();

        int32_t allocateDescriptors(int32_t numDescriptors) override;
        int32_t allocateSamplers(int32_t numSamplers) override;

        core::IndexTable _descriptor_table;
        core::IndexTable _sampler_table;

        UINT _descriptor_increment_size;
        UINT _sampler_increment_size;

        ComPtr < ID3D12DescriptorHeap> _cbvsrvuav_heap;
        ComPtr < ID3D12DescriptorHeap> _sampler_heap;

        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle(uint32_t offset) const;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle(uint32_t offset) const;

        D3D12_GPU_DESCRIPTOR_HANDLE gpuSamplerHandle(uint32_t offset) const;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuSamplerHandle(uint32_t offset) const;
    };

    class D3D12Backend : public DeviceBackend {
    public:
        D3D12Backend();
        virtual ~D3D12Backend();

        static const uint8_t CHAIN_NUM_FRAMES = 3;

        void* nativeDevice() override { return _device.Get(); }

        // DirectX 12 Objects
        ComPtr<ID3D12Device5> _device;
        ComPtr<ID3D12CommandQueue> _commandQueue;
        double _commandQueueTimestampFrequency = 1.0;

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
        BatchTimerPointer createBatchTimer(const BatchTimerInit& init) override;
 
        BufferPointer _createBuffer(const BufferInit& init, const std::string& name) override;

        TexturePointer createTexture(const TextureInit& init) override;

        GeometryPointer createGeometry(const GeometryInit& init) override;

        ShaderPointer createShader(const ShaderInit& init) override;
        ShaderPointer createProgram(const ProgramInit& init) override;

        SamplerPointer createSampler(const SamplerInit& init) override;

        PipelineStatePointer createGraphicsPipelineState(const GraphicsPipelineStateInit& init) override;
        PipelineStatePointer createComputePipelineState(const ComputePipelineStateInit& init) override;
        PipelineStatePointer createRaytracingPipelineState(const RaytracingPipelineStateInit& init) override;
   
        RootDescriptorLayoutPointer createRootDescriptorLayout(const RootDescriptorLayoutInit& init) override;
        DescriptorSetPointer createDescriptorSet(const DescriptorSetInit& init) override;

        ShaderEntry          getShaderEntry(const PipelineStatePointer& pipeline, const std::string& entry) override;
        ShaderTablePointer   createShaderTable(const ShaderTableInit& init) override;

        void updateDescriptorSet(DescriptorSetPointer& descriptorSet, DescriptorObjects& objects) override;
        DescriptorHeapPointer createDescriptorHeap(const DescriptorHeapInit& init) override;

        void executeBatch(const BatchPointer& batch) override;
        void presentSwapchain(const SwapchainPointer& swapchain) override;

        void flush() override;

        // Default empty pipeline layout
        RootDescriptorLayoutPointer _emptyRootDescriptorLayout;

        // Global Descriptor Heap
        DescriptorHeapPointer getDescriptorHeap() override;
        DescriptorHeapPointer _descriptorHeap;

        // Separate shader and pipeline state compilation as functions in order
        // to be able to live edit the shaders
        bool compileShader(Shader* shader, const std::string& src);
        bool compileShaderLib(Shader* shader, const std::string& src);
        bool realizePipelineState(PipelineState* pipeline);

        // When we modify d3d12 objects already in use, we need to garbage collect
        // them to be destroyed at a later time
        void garbageCollect(const ComPtr<ID3D12DeviceChild>& child);
        void flushGarbage();
        std::list< ComPtr<ID3D12DeviceChild> > _garbageObjects;

        // Enum translation tables
        static const DXGI_FORMAT Format[uint32_t(PixelFormat::COUNT)];

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

        // Gather color buffer rtvs in one heap
        UINT _numRenderTargets = 0;
        ComPtr<ID3D12DescriptorHeap> _rtvDescriptorHeap;
        UINT _rtvDescriptorSize = 0;
        D3D12_CPU_DESCRIPTOR_HANDLE _rtvs;

        ComPtr<ID3D12DescriptorHeap> _dsvDescriptorHeap;
        UINT _dsvDescriptorSize = 0;
        D3D12_CPU_DESCRIPTOR_HANDLE _dsv;
    };

    class D3D12BatchBackend : public Batch {
    public:
        friend class D3D12Backend;
        D3D12BatchBackend();
        virtual ~D3D12BatchBackend();

        void begin(uint8_t currentIndex, const BatchTimerPointer& timer) override;
        void end() override;

        void beginPass(const SwapchainPointer& swapchain, uint8_t currentIndex) override;
        void endPass() override;

        void clear(const SwapchainPointer& swapchain, uint8_t index, const core::vec4& color, float depth) override;
        void clear(const FramebufferPointer& framebuffer, const core::vec4& color, float depth) override;

        void resourceBarrierTransition(
            ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const SwapchainPointer& swapchain, uint8_t currentIndex, uint32_t subresource) override;
        void resourceBarrierTransition(
            ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const BufferPointer& buffer) override;
        void resourceBarrierTransition(
            ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const TexturePointer& texture, uint32_t subresource) override;

        void resourceBarrierRW(
            ResourceBarrierFlag flag, const BufferPointer& buffer) override;
        void resourceBarrierRW(
            ResourceBarrierFlag flag, const TexturePointer& texture, uint32_t subresource) override;

        void setViewport(const core::vec4& viewport) override;
        void setScissor(const core::vec4& scissor) override;

        void bindDescriptorHeap(const DescriptorHeapPointer& descriptorHeap);

        void bindFramebuffer(const FramebufferPointer& framebuffer) override;

        void bindRootDescriptorLayout(PipelineType type, const RootDescriptorLayoutPointer& rootDescriptorLayout) override;
        void bindPipeline(const PipelineStatePointer& pipeline) override;

        void bindDescriptorSet(PipelineType type, const DescriptorSetPointer& descriptorSet) override;
        void bindPushUniform(PipelineType type, uint32_t slot, uint32_t size, const uint8_t* data) override;

        void bindIndexBuffer(const BufferPointer& buffer) override;
        void bindVertexBuffers(uint32_t num, const BufferPointer* buffers) override;

        void draw(uint32_t numPrimitives, uint32_t startIndex) override;
        void drawIndexed(uint32_t numPrimitives, uint32_t startIndex) override;

        void uploadTexture(const TexturePointer& dest, const UploadSubresourceLayoutArray& subresourceLayout, const BufferPointer& src) override;

        void copyBufferRegion(const BufferPointer& dest, uint32_t destOffset, const BufferPointer& src, uint32_t srcOffset, uint32_t size) override;
        void uploadBuffer(const BufferPointer& dest) override;

        void dispatch(uint32_t numThreadsX, uint32_t numThreadsY, uint32_t numThreadsZ) override;

        void dispatchRays(const DispatchRaysArgs& args) override;

        ComPtr<ID3D12GraphicsCommandList4> _commandList;
        ComPtr<ID3D12CommandAllocator> _commandAllocators[D3D12Backend::CHAIN_NUM_FRAMES];

        DescriptorHeapPointer _descriptorHeap;

        UINT _currentBackBufferIndex{ 0 };

        UINT _currentGraphicsRootLayout_setRootIndex = 0;
        UINT _currentGraphicsRootLayout_samplerRootIndex = 0;
        UINT _currentComputeRootLayout_setRootIndex = 0;
        UINT _currentComputeRootLayout_samplerRootIndex = 0;

        BatchTimerPointer _timer;

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
        ComPtr<ID3D12Resource> _cpuDataResource;


        D3D12_CONSTANT_BUFFER_VIEW_DESC _uniformBufferView;
        D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;
        D3D12_INDEX_BUFFER_VIEW _indexBufferView;
        D3D12_SHADER_RESOURCE_VIEW_DESC _resourceBufferView;
        D3D12_UNORDERED_ACCESS_VIEW_DESC _rwResourceBufferView;
    };

    class D3D12TextureBackend : public Texture {
    public:
        friend class D3D12Backend;
        D3D12TextureBackend();
        virtual ~D3D12TextureBackend();

        ComPtr<ID3D12Resource> _resource;

        D3D12_SHADER_RESOURCE_VIEW_DESC   _shaderResourceViewDesc;
        D3D12_UNORDERED_ACCESS_VIEW_DESC   _unorderedAccessViewDesc;
    };

    class D3D12GeometryBackend : public Geometry {
    public:
        friend class D3D12Backend;
        D3D12GeometryBackend();
        virtual ~D3D12GeometryBackend();

        // Acceleration structure
        ComPtr<ID3D12Resource> _bottomLevelAccelerationStructure;
        ComPtr<ID3D12Resource> _topLevelAccelerationStructure;
    };

    class D3D12ShaderBackend : public Shader {
    public:
        friend class D3D12Backend;
        D3D12ShaderBackend();
        virtual ~D3D12ShaderBackend();

       // ComPtr<ID3DBlob> _shaderBlob;
        ComPtr<IDxcBlob> _shaderBlob;
    //    ComPtr<IDxcBlob> _shaderLibBlob;

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

        ComPtr<ID3D12StateObject> _stateObject; // raytracing
        ComPtr<ID3D12RootSignature> _localRootSignature; // raytracing

        D3D12_PRIMITIVE_TOPOLOGY _primitive_topology;

        static void fill_rasterizer_desc(const RasterizerState& src, D3D12_RASTERIZER_DESC& dst);
        static void fill_depth_stencil_desc(const DepthStencilState& src, D3D12_DEPTH_STENCIL_DESC& dst);
        static void fill_blend_desc(const BlendState& src, D3D12_BLEND_DESC& dst);
    };


    class D3D12RootDescriptorLayoutBackend : public RootDescriptorLayout {
    public:
        friend class D3D12Backend;
        D3D12RootDescriptorLayoutBackend();
        virtual ~D3D12RootDescriptorLayoutBackend();

        ComPtr<ID3D12RootSignature> _rootSignature;
      
        std::vector< uint32_t > _dxPushParamIndices;
        
        int32_t _cbvsrvuav_rootIndex = -1;
        std::vector< uint32_t > _dxSetParamIndices;

        int32_t _sampler_rootIndex = -1;

        uint32_t _push_count = 0;
        uint32_t _cbvsrvuav_count = 0;
        std::vector< uint32_t > _cbvsrvuav_counts;
        uint32_t _sampler_count = 0;
    };

    class D3D12DescriptorSetBackend : public DescriptorSet {
    public:
        friend class D3D12Backend;
        D3D12DescriptorSetBackend();
        virtual ~D3D12DescriptorSetBackend();

        int32_t _cbvsrvuav_rootIndex = -1;
        int32_t _sampler_rootIndex = -1;

        uint32_t _cbvsrvuav_count = 0;
        uint32_t _sampler_count = 0;

        D3D12_GPU_DESCRIPTOR_HANDLE _cbvsrvuav_GPUHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE _sampler_GPUHandle;
    };

    class D3D12ShaderTableBackend : public ShaderTable {
    public:
        friend class D3D12Backend;
        D3D12ShaderTableBackend();
        virtual ~D3D12ShaderTableBackend();

        ComPtr<ID3D12Resource> _shaderTable;
    };

    class D3D12BatchTimerBackend : public BatchTimer {
    public:
        friend class D3D12Backend;
        D3D12BatchTimerBackend();
        virtual ~D3D12BatchTimerBackend();

        void begin(ID3D12GraphicsCommandList* _commandList, INT index);
        void end(ID3D12GraphicsCommandList* _commandList, INT index);

        ComPtr<ID3D12QueryHeap> _queryHeap;
        ComPtr<ID3D12Resource> _queryResult;
        ComPtr<ID3D12Resource> _queryResultMapped;
        void* _queryResultCPUMappedAddress = nullptr;

        double _commandQueueTimestampFrequency = 1.0;
    };
}

#endif _WINDOWS
