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

        ComPtr<ID3D12GraphicsCommandList> g_CommandList;
        ComPtr<ID3D12CommandAllocator> g_CommandAllocators[CHAIN_NUM_FRAMES];
        ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;

        ComPtr<IDXGISwapChain4> g_SwapChain;
        ComPtr<ID3D12Resource> g_BackBuffers[CHAIN_NUM_FRAMES];
    };

}

#endif WIN32
