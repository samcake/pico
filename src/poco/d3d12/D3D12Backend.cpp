// D3D12Backend.cpp
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
#include "D3D12Backend.h"
#include "Api.h"

#include <chrono>

#ifdef WIN32
//d3d12.lib dxgi.lib dxguid.lib D3DCompiler.lib Shlwapi.lib
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "Shlwapi.lib")

// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

using namespace poco;


#define ThrowIfFailed(result) if (FAILED((result))) pocoLog() << "FAILED !!!/n";
/**
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        pocoLog() << "This failed ?";
        throw std::exception();
    }
}*/

const D3D12_RESOURCE_STATES D3D12BatchBackend::ResourceStates[] = {
    D3D12_RESOURCE_STATE_COMMON,
    D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
    D3D12_RESOURCE_STATE_INDEX_BUFFER,
    D3D12_RESOURCE_STATE_RENDER_TARGET,
    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
    D3D12_RESOURCE_STATE_DEPTH_WRITE,
    D3D12_RESOURCE_STATE_DEPTH_READ,
    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    D3D12_RESOURCE_STATE_STREAM_OUT,
    D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
    D3D12_RESOURCE_STATE_COPY_DEST,
    D3D12_RESOURCE_STATE_COPY_SOURCE,
    D3D12_RESOURCE_STATE_RESOLVE_DEST,
    D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
    D3D12_RESOURCE_STATE_GENERIC_READ,
    D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
    D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE,
    D3D12_RESOURCE_STATE_PRESENT,
    D3D12_RESOURCE_STATE_PREDICATION,
    D3D12_RESOURCE_STATE_VIDEO_DECODE_READ,
    D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE,
    D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ,
    D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE,
    D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ,
    D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE
};

const D3D12_RESOURCE_BARRIER_FLAGS D3D12BatchBackend::ResourceBarrieFlags[] = {
    D3D12_RESOURCE_BARRIER_FLAG_NONE,
    D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY,
    D3D12_RESOURCE_BARRIER_FLAG_END_ONLY,
};


ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp)
{
    ComPtr<IDXGIFactory4> dxgiFactory;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

    ComPtr<IDXGIAdapter1> dxgiAdapter1;
    ComPtr<IDXGIAdapter4> dxgiAdapter4;

    if (useWarp)
    {
        ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
        ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
    }
    else
    {
        SIZE_T maxDedicatedVideoMemory = 0;
        for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
            dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

            // Check to see if the adapter can create a D3D12 device without actually 
            // creating it. The adapter with the largest dedicated video memory
            // is favored.
            if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
                SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
                    D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
                dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
            {
                maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
            }
        }
    }

    return dxgiAdapter4;
}

void EnableDebugLayer()
{
#if defined(_DEBUG)
    // Always enable the debug layer before doing anything DX12 related
    // so all possible errors generated while creating DX12 objects
    // are caught by the debug layer.
    ComPtr<ID3D12Debug> debugInterface;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();
#endif
}

ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter)
{

    EnableDebugLayer();

    ComPtr<ID3D12Device2> d3d12Device2;
    ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));

    // Enable debug messages in debug mode.
#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
    {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        // Suppress whole categories of messages
        //D3D12_MESSAGE_CATEGORY Categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY Severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID DenyIds[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
        };

        D3D12_INFO_QUEUE_FILTER NewFilter = {};
        //NewFilter.DenyList.NumCategories = _countof(Categories);
        //NewFilter.DenyList.pCategoryList = Categories;
        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs = _countof(DenyIds);
        NewFilter.DenyList.pIDList = DenyIds;

        ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
    }
#endif

    return d3d12Device2;
}


ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

    return d3d12CommandQueue;
}

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
/*
void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device,
    ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap)
{
    auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

    for (int i = 0; i < g_NumFrames; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

        g_BackBuffers[i] = backBuffer;

        rtvHandle.Offset(rtvDescriptorSize);
    }
}
*/

D3D12SwapchainBackend::D3D12SwapchainBackend() : Swapchain() {

}

D3D12SwapchainBackend::~D3D12SwapchainBackend() {

}

SwapchainPointer D3D12Backend::createSwapchain(const SwapchainInit& init) {
    auto sw = new D3D12SwapchainBackend();

    sw->g_SwapChain = CreateSwapChain(init.hWnd, g_CommandQueue, init.width, init.height, D3D12Backend::CHAIN_NUM_FRAMES);


    sw->_currentIndex = sw->g_SwapChain->GetCurrentBackBufferIndex();

    sw->g_RTVDescriptorHeap = CreateDescriptorHeap(g_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12Backend::CHAIN_NUM_FRAMES);
    sw->g_RTVDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    auto rtvDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = sw->g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    for (int i = 0; i < D3D12Backend::CHAIN_NUM_FRAMES; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        ThrowIfFailed(sw->g_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        g_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

        sw->g_BackBuffers[i] = backBuffer;

        rtvHandle.ptr += (rtvDescriptorSize);
    }

    return SwapchainPointer(sw);
}



ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device,
    D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

    return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device,
    ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

    ThrowIfFailed(commandList->Close());

    return commandList;
}

ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device)
{
    ComPtr<ID3D12Fence> fence;

    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

    return fence;
}

HANDLE CreateEventHandle()
{
    HANDLE fenceEvent;

    fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
   // assert(fenceEvent && "Failed to create fence event.");

    return fenceEvent;
}

uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence,
    uint64_t& fenceValue)
{
    uint64_t fenceValueForSignal = ++fenceValue;
    ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValueForSignal));

    return fenceValueForSignal;
}

void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent,
    std::chrono::milliseconds duration = std::chrono::milliseconds::max())
{
    if (fence->GetCompletedValue() < fenceValue)
    {
        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
        ::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
    }
}

void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence,
    uint64_t& fenceValue, HANDLE fenceEvent)
{
    uint64_t fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
    WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
}


BatchPointer D3D12Backend::createBatch(const BatchInit& init) {

    auto batch = new D3D12BatchBackend();

    for (int i = 0; i < D3D12Backend::CHAIN_NUM_FRAMES; ++i)
    {
        batch->g_CommandAllocators[i] = CreateCommandAllocator(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
    batch->g_CommandList = CreateCommandList(g_Device,
        batch->g_CommandAllocators[batch->g_CurrentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);

    return BatchPointer(batch);
}
D3D12BatchBackend::D3D12BatchBackend() :
    Batch()
{
}

D3D12BatchBackend::~D3D12BatchBackend() {
}

void D3D12BatchBackend::begin(uint8_t currentIndex) {
    g_CurrentBackBufferIndex = currentIndex;
    auto commandAllocator = g_CommandAllocators[currentIndex];

    commandAllocator->Reset();
    g_CommandList->Reset(commandAllocator.Get(), nullptr);
}

void D3D12BatchBackend::clear(const vec4& color, const SwapchainPointer& swapchain, uint8_t index) {

    auto sw = static_cast<D3D12SwapchainBackend*>(swapchain.get());

    FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
    D3D12_CPU_DESCRIPTOR_HANDLE rtv { sw->g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + sw->g_RTVDescriptorSize * index};

    g_CommandList->ClearRenderTargetView(rtv, color.data(), 0, nullptr);
}

void D3D12BatchBackend::resourceBarrierTransition(
    BarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
    const SwapchainPointer& swapchain, uint8_t currentIndex, uint32_t subresource) {

    auto sw = static_cast<D3D12SwapchainBackend*>(swapchain.get());
    auto backBuffer = sw->g_BackBuffers[currentIndex];

    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = ResourceBarrieFlags[uint32_t(flag)];
    barrier.Transition.pResource = backBuffer.Get();
    barrier.Transition.StateBefore = ResourceStates[uint32_t(stateBefore)];
    barrier.Transition.StateAfter = ResourceStates[uint32_t(stateAfter)];
    barrier.Transition.Subresource = (subresource == -1 ? D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES : subresource);

    g_CommandList->ResourceBarrier(1, &barrier);
}


void D3D12BatchBackend::end() {
    ThrowIfFailed(g_CommandList->Close());
}









D3D12Backend::D3D12Backend() {
    ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(false);

    g_Device = CreateDevice(dxgiAdapter4);

    g_CommandQueue = CreateCommandQueue(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    g_Fence = CreateFence(g_Device);
    g_FenceEvent = CreateEventHandle();
}

D3D12Backend::~D3D12Backend() {

}

void D3D12Backend::executeBatch(const BatchPointer& batch) {
    auto bat = static_cast<D3D12BatchBackend*>(batch.get());


    ID3D12CommandList* const commandLists[] = {
        bat->g_CommandList.Get()
    };
    g_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

}

void D3D12Backend::presentSwapchain(const SwapchainPointer& swapchain) {
    auto sw = static_cast<D3D12SwapchainBackend*>(swapchain.get());

    // UINT syncInterval = g_VSync ? 1 : 0;
    UINT syncInterval = 1;
    // UINT presentFlags = g_TearingSupported && !g_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    UINT presentFlags = 0;
    ThrowIfFailed(sw->g_SwapChain->Present(syncInterval, presentFlags));

    g_FrameFenceValues[sw->currentIndex()] = Signal(g_CommandQueue, g_Fence, g_FenceValue);

    sw->_currentIndex = sw->g_SwapChain->GetCurrentBackBufferIndex();

    WaitForFenceValue(g_Fence, g_FrameFenceValues[sw->_currentIndex], g_FenceEvent);
}

#endif
