// D3D12Backend_Framebuffer.cpp
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
#include "D3D12Backend.h"
#include "Api.h"

#include <vector>

using namespace poco;

#ifdef WIN32

D3D12FramebufferBackend::D3D12FramebufferBackend() : Framebuffer() {

}

D3D12FramebufferBackend::~D3D12FramebufferBackend() {

}

FramebufferPointer D3D12Backend::createFramebuffer(const FramebufferInit& init) {
    auto framebuffer = new D3D12FramebufferBackend();
/*
    sw->_swapchain = CreateSwapChain(init.hWnd, _commandQueue, init.width, init.height, D3D12Backend::CHAIN_NUM_FRAMES);


    sw->_currentIndex = sw->_swapchain->GetCurrentBackBufferIndex();

    sw->_rtvDescriptorHeap = CreateDescriptorHeap(_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12Backend::CHAIN_NUM_FRAMES);
    sw->_rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    auto rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = sw->_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    for (int i = 0; i < D3D12Backend::CHAIN_NUM_FRAMES; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        ThrowIfFailed(sw->_swapchain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        _device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

        sw->_backBuffers[i] = backBuffer;

        rtvHandle.ptr += (rtvDescriptorSize);
    }
*/
    return FramebufferPointer(framebuffer);
}

#endif