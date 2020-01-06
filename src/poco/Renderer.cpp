// Renderer.cpp
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
#include "Renderer.h"

#include "Device.h"
#include "Batch.h"
#include "Swapchain.h"

using namespace poco;

Renderer::Renderer(const DevicePointer& device) :
    _device(device)
{
    poco::BatchInit batchInit{};
    _batch= _device->createBatch(batchInit);

}

Renderer::~Renderer() {

}

void Renderer::render(const CameraPointer& camera, SwapchainPointer& swapchain) {
    auto currentIndex = swapchain->currentIndex();
    _batch->begin(currentIndex);

    /*
    auto commandAllocator = g_CommandAllocators[g_CurrentBackBufferIndex];

    commandAllocator->Reset();
    g_CommandList->Reset(commandAllocator.Get(), nullptr);
    */
    _batch->resourceBarrierTransition(
        poco::Batch::BarrierFlag::NONE,
        poco::Batch::ResourceState::PRESENT,
        poco::Batch::ResourceState::RENDER_TARGET,
        swapchain, currentIndex, -1);
    /*
        auto backBuffer = g_BackBuffers[g_CurrentBackBufferIndex];
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffer.Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    g_CommandList->ResourceBarrier(1, &barrier);
*/
    _batch->clear(swapchain, currentIndex);
    // Clear the render target.
    /*{
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            g_CurrentBackBufferIndex, g_RTVDescriptorSize);

        g_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }*/

    _batch->resourceBarrierTransition(
        poco::Batch::BarrierFlag::NONE,
        poco::Batch::ResourceState::RENDER_TARGET,
        poco::Batch::ResourceState::PRESENT,
        swapchain, currentIndex, -1);
/*    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffer.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    g_CommandList->ResourceBarrier(1, &barrier);
*/
    _batch->end();
    //ThrowIfFailed(g_CommandList->Close());

    _device->executeBatch(_batch);
    /*
    ID3D12CommandList* const commandLists[] = {
        g_CommandList.Get()
    };
    g_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
*/

    _device->presentSwapchain(swapchain);
/*
// Present
{
    UINT syncInterval = g_VSync ? 1 : 0;
    UINT presentFlags = g_TearingSupported && !g_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    ThrowIfFailed(g_SwapChain->Present(syncInterval, presentFlags));

    g_FrameFenceValues[g_CurrentBackBufferIndex] = Signal(g_CommandQueue, g_Fence, g_FenceValue);

    g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

    WaitForFenceValue(g_Fence, g_FrameFenceValues[g_CurrentBackBufferIndex], g_FenceEvent);
}
*/

}
