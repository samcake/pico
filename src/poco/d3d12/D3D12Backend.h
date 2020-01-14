// D3D12Backend.h 
//
// Sam Gateau - 2020/1/1
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

#include "../gpu/Device.h"
#include "../gpu/Swapchain.h"
#include "../gpu/Batch.h"
#include "../gpu/Resource.h"
#include "../gpu/Pipeline.h"
#include "../gpu/Shader.h"

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

namespace poco {
    
    class D3D12Backend : public DeviceBackend {
    public:

        D3D12Backend();
        virtual ~D3D12Backend();

        static const uint8_t CHAIN_NUM_FRAMES = 3;

        // DirectX 12 Objects
        ComPtr<ID3D12Device2> _device;
        ComPtr<ID3D12CommandQueue> _commandQueue;

        // Synchronization objects
        ComPtr<ID3D12Fence> _fence;
        uint64_t _fenceValue = 0;
        uint64_t _frameFenceValues[D3D12Backend::CHAIN_NUM_FRAMES] = {};
        HANDLE _fenceEvent;

        SwapchainPointer createSwapchain(const SwapchainInit& init) override;
        BatchPointer createBatch(const BatchInit& init) override;
        BufferPointer createBuffer(const BufferInit& init) override;

        ShaderPointer createShader(const ShaderInit& init) override;
        ShaderPointer createProgram(const ProgramInit& init) override;

        PipelineStatePointer createPipelineState(const PipelineStateInit& init) override;

        void executeBatch(const BatchPointer& batch) override;
        void presentSwapchain(const SwapchainPointer& swapchain) override;
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

        void clear(const vec4& color, const SwapchainPointer& swapchain, uint8_t index) override;
        void resourceBarrierTransition(
            ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const SwapchainPointer& swapchain, uint8_t currentIndex, uint32_t subresource) override;

        void setViewport(vec4& viewport) override;
        void setScissor(vec4& scissor) override;

        void setPipeline(PipelineStatePointer pipeline) override;

        void bindIndexBuffer(BufferPointer& buffer) override;
        void bindVertexBuffers(uint32_t num, BufferPointer* buffers) override;

        void drawIndexed(uint32_t numPrimitives, uint32_t startIndex) override;

        ComPtr<ID3D12GraphicsCommandList> _commandList;
        ComPtr<ID3D12CommandAllocator> _commandAllocators[D3D12Backend::CHAIN_NUM_FRAMES];

        UINT _currentBackBufferIndex { 0 };



        static const D3D12_RESOURCE_STATES ResourceStates[uint32_t(ResourceState::COUNT)];
        static const D3D12_RESOURCE_BARRIER_FLAGS  ResourceBarrieFlags[uint32_t(ResourceBarrierFlag::COUNT)];
    };


    class D3D12BufferBackend : public Buffer {
    public:
        friend class D3D12Backend;
        D3D12BufferBackend();
        virtual ~D3D12BufferBackend();

        ComPtr<ID3D12Resource> _buffer;


        D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;
        D3D12_INDEX_BUFFER_VIEW _indexBufferView;

    //    ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;
    //    UINT g_RTVDescriptorSize;
    };

    class D3D12ShaderBackend : public Shader {
    public:
        friend class D3D12Backend;
        D3D12ShaderBackend();
        virtual ~D3D12ShaderBackend();

        ComPtr<ID3DBlob> _shaderBlob;
        //    UINT g_RTVDescriptorSize;


        static const std::string ShaderTypes[uint32_t(ShaderType::COUNT)];

    };

    class D3D12PipelineStateBackend : public PipelineState {
    public:
        friend class D3D12Backend;
        D3D12PipelineStateBackend();
        virtual ~D3D12PipelineStateBackend();

        ComPtr<ID3D12RootSignature> _rootSignature;
        ComPtr<ID3D12PipelineState> _pipelineState;
        //    ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;
        //    UINT g_RTVDescriptorSize;
    };

}

#endif WIN32
