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

#include "Device.h"
#include "Swapchain.h"
#include "Batch.h"

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
        ComPtr<ID3D12Device2> g_Device;
        ComPtr<ID3D12CommandQueue> g_CommandQueue;

        // Synchronization objects
        ComPtr<ID3D12Fence> g_Fence;
        uint64_t g_FenceValue = 0;
        uint64_t g_FrameFenceValues[D3D12Backend::CHAIN_NUM_FRAMES] = {};
        HANDLE g_FenceEvent;


        SwapchainPointer createSwapchain(const SwapchainInit& init) override;
        BatchPointer createBatch(const BatchInit& init) override;


        void executeBatch(const BatchPointer& batch) override;
        void presentSwapchain(const SwapchainPointer& swapchain) override;
    };


    class D3D12SwapchainBackend : public Swapchain {
    public:
        D3D12SwapchainBackend();
        virtual ~D3D12SwapchainBackend();


        ComPtr<IDXGISwapChain4> g_SwapChain;
        ComPtr<ID3D12Resource> g_BackBuffers[D3D12Backend::CHAIN_NUM_FRAMES];

        ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;
        UINT g_RTVDescriptorSize;
    };

    class D3D12BatchBackend : public Batch {
    public:
        D3D12BatchBackend();
        virtual ~D3D12BatchBackend();

        void begin(uint8_t currentIndex) override;
        void clear(const SwapchainPointer& swapchain, uint8_t index) override;
        void resourceBarrierTransition(
            BarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
            const SwapchainPointer& swapchain, uint8_t currentIndex, uint32_t subresource) override;

        void end() override;

        ComPtr<ID3D12GraphicsCommandList> g_CommandList;
        ComPtr<ID3D12CommandAllocator> g_CommandAllocators[D3D12Backend::CHAIN_NUM_FRAMES];

        UINT g_CurrentBackBufferIndex { 0 };



        static const D3D12_RESOURCE_STATES ResourceStates[uint32_t(Batch::ResourceState::COUNT)];
        static const D3D12_RESOURCE_BARRIER_FLAGS  ResourceBarrieFlags[uint32_t(Batch::BarrierFlag::COUNT)];
    };

}

#endif WIN32
