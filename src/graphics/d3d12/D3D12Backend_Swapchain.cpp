// D3D12Backend_Descriptor.cpp
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

#include <algorithm>

using namespace graphics;

#ifdef _WINDOWS
#define ThrowIfFailed(result) if (FAILED((result))) picoLog() << "FAILED !!!/n";

// D3D12SwapchainBackend

bool CheckTearingSupport()
{
    BOOL allowTearing = FALSE;

    // Rather than create the DXGI 1.5 factory interface directly, we create the
    // DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
    // graphics debugging tools which will not support the 1.5 factory interface 
    // until a future update.
    ComPtr<IDXGIFactory4> factory4;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
    {
        ComPtr<IDXGIFactory5> factory5;
        if (SUCCEEDED(factory4.As(&factory5)))
        {
            if (FAILED(factory5->CheckFeatureSupport(
                DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                &allowTearing, sizeof(allowTearing))))
            {
                allowTearing = FALSE;
            }
        }
    }

    return allowTearing == TRUE;
}

ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd,
    ComPtr<ID3D12CommandQueue> commandQueue,
    uint32_t width, uint32_t height, uint32_t bufferCount)
{
    ComPtr<IDXGISwapChain4> dxgiSwapChain4;
    ComPtr<IDXGIFactory4> dxgiFactory4;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = bufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    // It is recommended to always allow tearing if tearing support is available.
    swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
        commandQueue.Get(),
        hWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1));

    // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
    // will be handled manually.
    ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

    return dxgiSwapChain4;
}

ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device,
    D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;

    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
}


bool CreateSwapchainRenderTargets(ComPtr<ID3D12Device2> device, D3D12SwapchainBackend* sw) {

    sw->_rtvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12Backend::CHAIN_NUM_FRAMES);
    sw->_rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = sw->_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    // Setup RTV descriptor to specify sRGB format.
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
   // rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    for (int i = 0; i < D3D12Backend::CHAIN_NUM_FRAMES; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        ThrowIfFailed(sw->_swapchain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));


        device->CreateRenderTargetView(backBuffer.Get(), &rtvDesc, rtvHandle);

        sw->_backBuffers[i] = backBuffer;

        rtvHandle.ptr += (sw->_rtvDescriptorSize);
    }

    return true;
}

bool CreateDepthStencilBuffer(ComPtr<ID3D12Device2> device, uint32_t width, uint32_t height, ComPtr<ID3D12Resource>& depthBuffer, ComPtr<ID3D12DescriptorHeap>& dsvDescriptorHeap, UINT& dsvDescriptorSize) {
    // Start fresh for sure
    depthBuffer.Reset();
    dsvDescriptorHeap.Reset();
    dsvDescriptorSize = 0;

    // Create a depth buffer.
    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil = { 0.0f, 0 }; // Default clear value is 0.f because we use Inverted Z and 0 is matching Far

    D3D12_HEAP_PROPERTIES heapProp;
    heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProp.CreationNodeMask = 1;
    heapProp.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 0;
    desc.Format = DXGI_FORMAT_D32_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    device->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optimizedClearValue,
        IID_PPV_ARGS(&depthBuffer)
    );

    dsvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
    dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    // Update the depth-stencil view.
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    dsv.Flags = D3D12_DSV_FLAG_NONE;

    device->CreateDepthStencilView(depthBuffer.Get(), &dsv, dsvHandle);

    return true;
}


D3D12SwapchainBackend::D3D12SwapchainBackend() : Swapchain() {

}

D3D12SwapchainBackend::~D3D12SwapchainBackend() {

}

SwapchainPointer D3D12Backend::createSwapchain(const SwapchainInit& init) {
    auto sw = new D3D12SwapchainBackend();

    sw->_init = init;
    sw->_swapchain = CreateSwapChain(init.hWnd, _commandQueue, init.width, init.height, D3D12Backend::CHAIN_NUM_FRAMES);
    sw->_currentIndex = sw->_swapchain->GetCurrentBackBufferIndex();

    CreateSwapchainRenderTargets(_device, sw);

    if (init.depthBuffer) {
        CreateDepthStencilBuffer(_device, init.width, init.height, sw->_depthBuffer, sw->_dsvDescriptorHeap, sw->_dsvDescriptorSize);
    }

    return SwapchainPointer(sw);
}


void D3D12Backend::resizeSwapchain(const SwapchainPointer& swapchain, uint32_t width, uint32_t height) {

    auto sw = static_cast<D3D12SwapchainBackend*>(swapchain.get());

    // Update the client size.
    if (sw->_init.width != width || sw->_init.height != height)
    {
        sw->_init.width = std::max(1U, width);
        sw->_init.height = std::max(1U, height);

        flush();

        for (int i = 0; i < D3D12Backend::CHAIN_NUM_FRAMES; ++i)
        {
            sw->_backBuffers[i].Reset();
        }

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        ThrowIfFailed(sw->_swapchain->GetDesc(&swapChainDesc));
        ThrowIfFailed(sw->_swapchain->ResizeBuffers(D3D12Backend::CHAIN_NUM_FRAMES, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));
        sw->_currentIndex = sw->_swapchain->GetCurrentBackBufferIndex();

        CreateSwapchainRenderTargets(_device, sw);

        if (sw->_init.depthBuffer) {
            CreateDepthStencilBuffer(_device, sw->_init.width, sw->_init.height, sw->_depthBuffer, sw->_dsvDescriptorHeap, sw->_dsvDescriptorSize);
        }
    }
}


#endif
