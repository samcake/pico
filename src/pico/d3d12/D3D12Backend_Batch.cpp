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
#include "Api.h"

using namespace pico;

#ifdef WIN32
#define ThrowIfFailed(result) if (FAILED((result))) picoLog() << "FAILED !!!/n";



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


BatchPointer D3D12Backend::createBatch(const BatchInit & init) {

    auto batch = new D3D12BatchBackend();

    for (int i = 0; i < D3D12Backend::CHAIN_NUM_FRAMES; ++i)
    {
        batch->_commandAllocators[i] = CreateCommandAllocator(_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
    batch->_commandList = CreateCommandList(_device,
        batch->_commandAllocators[batch->_currentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);

    return BatchPointer(batch);
}

D3D12BatchBackend::D3D12BatchBackend() :
    Batch()
{
}

D3D12BatchBackend::~D3D12BatchBackend() {
}

void D3D12BatchBackend::begin(uint8_t currentIndex) {
    _currentBackBufferIndex = currentIndex;
    auto commandAllocator = _commandAllocators[currentIndex];

    commandAllocator->Reset();
    _commandList->Reset(commandAllocator.Get(), nullptr);
}

void D3D12BatchBackend::end() {
    ThrowIfFailed(_commandList->Close());
}

void D3D12BatchBackend::beginPass(const SwapchainPointer & swapchain, uint8_t index) {
    auto sw = static_cast<D3D12SwapchainBackend*>(swapchain.get());
    D3D12_CPU_DESCRIPTOR_HANDLE rtv{ sw->_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + sw->_rtvDescriptorSize * index };
    
    if (sw->_dsvDescriptorHeap) {
        D3D12_CPU_DESCRIPTOR_HANDLE dsv{ sw->_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr};
        _commandList->OMSetRenderTargets(1, &rtv, TRUE, &dsv);
    } else {
        _commandList->OMSetRenderTargets(1, &rtv, TRUE, nullptr);
    }
}

void D3D12BatchBackend::endPass() {
    _commandList->OMSetRenderTargets(0, nullptr, TRUE, nullptr); // needed ?

}

void D3D12BatchBackend::clear(const SwapchainPointer& swapchain, uint8_t index, const core::vec4& color, float depth) {

    auto sw = static_cast<D3D12SwapchainBackend*>(swapchain.get());

    D3D12_CPU_DESCRIPTOR_HANDLE rtv{ sw->_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + sw->_rtvDescriptorSize * index };
    _commandList->ClearRenderTargetView(rtv, color.data(), 0, nullptr);

    if (sw->_dsvDescriptorHeap) {
        D3D12_CPU_DESCRIPTOR_HANDLE dsv{ sw->_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr };
        _commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
    }
}

void D3D12BatchBackend::resourceBarrierTransition(
    ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
    const SwapchainPointer & swapchain, uint8_t currentIndex, uint32_t subresource) {

    auto sw = static_cast<D3D12SwapchainBackend*>(swapchain.get());
    auto backBuffer = sw->_backBuffers[currentIndex];

    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = ResourceBarrieFlags[uint32_t(flag)];
    barrier.Transition.pResource = backBuffer.Get();
    barrier.Transition.StateBefore = ResourceStates[uint32_t(stateBefore)];
    barrier.Transition.StateAfter = ResourceStates[uint32_t(stateAfter)];
    barrier.Transition.Subresource = (subresource == -1 ? D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES : subresource);

    _commandList->ResourceBarrier(1, &barrier);
}


void D3D12BatchBackend::setViewport(core::vec4 & viewport) {
    D3D12_VIEWPORT dxViewport;
    dxViewport.TopLeftX = viewport.x;
    dxViewport.TopLeftY = viewport.y;
    dxViewport.Width = viewport.z;
    dxViewport.Height = viewport.w;
    dxViewport.MaxDepth = 1.0f;
    dxViewport.MinDepth = 0.0f;

    _commandList->RSSetViewports(1, &dxViewport);
}

void D3D12BatchBackend::setScissor(core::vec4 & scissor) {
    D3D12_RECT dxRect;
    dxRect.left = scissor.x;
    dxRect.top = scissor.y;
    dxRect.right = scissor.x + scissor.z;
    dxRect.bottom = scissor.y + scissor.w;

    _commandList->RSSetScissorRects(1, &dxRect);
}

void D3D12BatchBackend::setPipeline(PipelineStatePointer pipeline) {
    auto dpso = static_cast<D3D12PipelineStateBackend*>(pipeline.get());

    auto dxPso = dpso->_pipelineState;
    _commandList->SetPipelineState(dxPso.Get());

    auto dxGRS = dpso->_rootSignature;
    _commandList->SetGraphicsRootSignature(dxGRS.Get());

    _commandList->IASetPrimitiveTopology(dpso->_primitive_topology);
}

void D3D12BatchBackend::bindDescriptorSet(DescriptorSetPointer descriptorSet) {
    auto dxds = static_cast<D3D12DescriptorSetBackend*>(descriptorSet.get());

    uint32_t descriptor_heap_count = 0;
    ID3D12DescriptorHeap* descriptor_heaps[2];
    if (NULL != dxds->_cbvsrvuav_heap) {
        descriptor_heaps[descriptor_heap_count] = dxds->_cbvsrvuav_heap.Get();
        ++descriptor_heap_count;
    }
    if (NULL != dxds->_sampler_heap) {
        descriptor_heaps[descriptor_heap_count] = dxds->_sampler_heap.Get();
        ++descriptor_heap_count;
    }

    if (descriptor_heap_count > 0) {
        _commandList->SetDescriptorHeaps(descriptor_heap_count, descriptor_heaps);
    }

    for (uint32_t i = 0; i < dxds->_dxRootParameterIndices.size(); ++i) {
        auto rooParameterIndex = dxds->_dxRootParameterIndices[i];
        if (-1 == rooParameterIndex) {
            continue;
        }

        _commandList->SetGraphicsRootDescriptorTable(rooParameterIndex, dxds->_dxGPUHandles[i]);

/*
        switch (p_descriptor_set->descriptors[i].type) {
        case tr_descriptor_type_sampler: {
            D3D12_GPU_DESCRIPTOR_HANDLE handle = p_descriptor_set->dx_sampler_heap->GetGPUDescriptorHandleForHeapStart();
            UINT handle_inc_size = p_cmd->cmd_pool->renderer->dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
            p_cmd->dx_cmd_list->SetGraphicsRootDescriptorTable(descriptor->dx_root_parameter_index, handle);
        }
                                       break;

        case tr_descriptor_type_uniform_buffer_cbv:
        case tr_descriptor_type_storage_buffer_srv:
        case tr_descriptor_type_storage_buffer_uav:
        case tr_descriptor_type_texture_srv:
        case tr_descriptor_type_texture_uav:
        case tr_descriptor_type_uniform_texel_buffer_srv:
        case tr_descriptor_type_storage_texel_buffer_uav: {
            D3D12_GPU_DESCRIPTOR_HANDLE handle = p_descriptor_set->dx_cbvsrvuav_heap->GetGPUDescriptorHandleForHeapStart();
            UINT handle_inc_size = p_cmd->cmd_pool->renderer->dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
            if (p_pipeline->type == tr_pipeline_type_graphics) {
                p_cmd->dx_cmd_list->SetGraphicsRootDescriptorTable(descriptor->dx_root_parameter_index, handle);
            }
            else if (p_pipeline->type == tr_pipeline_type_compute) {
                p_cmd->dx_cmd_list->SetComputeRootDescriptorTable(descriptor->dx_root_parameter_index, handle);
            }
        }
                                                        break;
        }
*/
    }
    
}

void D3D12BatchBackend::bindIndexBuffer(BufferPointer & buffer) {
    auto dbBuffer = static_cast<D3D12BufferBackend*>(buffer.get());
    _commandList->IASetIndexBuffer(&dbBuffer->_indexBufferView);
}

void D3D12BatchBackend::bindVertexBuffers(uint32_t num, BufferPointer * buffers) {
    auto dbBuffer = static_cast<D3D12BufferBackend*>(buffers[0].get());
    _commandList->IASetVertexBuffers(0, 1, &dbBuffer->_vertexBufferView);
}

void D3D12BatchBackend::draw(uint32_t numPrimitives, uint32_t startIndex) {
    _commandList->DrawInstanced(numPrimitives, 1, startIndex, 0);
}


void D3D12BatchBackend::drawIndexed(uint32_t numPrimitives, uint32_t startIndex) {
    _commandList->DrawIndexedInstanced(numPrimitives, 1, startIndex, 0, 0);
}



#endif
