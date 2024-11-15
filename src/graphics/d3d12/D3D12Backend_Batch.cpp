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

#include <core/api.h>

using namespace graphics;

#ifdef _WINDOWS
#define ThrowIfFailed(result) if (FAILED((result))) picoLog("D3D12Backend_Batch FAILED !!!");



ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device,
    D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

    return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList4> CreateCommandList(ComPtr<ID3D12Device5> device,
    ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12GraphicsCommandList4> commandList;
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

    batch->_descriptorHeap = _descriptorHeap;

    return BatchPointer(batch);
}

D3D12BatchBackend::D3D12BatchBackend() :
    Batch()
{
}

D3D12BatchBackend::~D3D12BatchBackend() {
}

void D3D12BatchBackend::begin(uint8_t currentIndex, const BatchTimerPointer& timer) {
    _currentBackBufferIndex = currentIndex;
    auto commandAllocator = _commandAllocators[currentIndex];

    commandAllocator->Reset();
    _commandList->Reset(commandAllocator.Get(), nullptr);

    if (timer) {
        _timer = timer;
        auto t = static_cast<D3D12BatchTimerBackend*>(timer.get());
        t->begin(_commandList.Get(), _currentBackBufferIndex);
    }
}

void D3D12BatchBackend::end() {
    if (_timer) {
        auto t = static_cast<D3D12BatchTimerBackend*>(_timer.get());
        t->end(_commandList.Get(), _currentBackBufferIndex);
        _timer.reset();
    }
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


    bindDescriptorHeap(_descriptorHeap);

    _currentGraphicsRootLayout_setRootIndex = 0;
    _currentGraphicsRootLayout_samplerRootIndex = 0;
    _currentComputeRootLayout_setRootIndex = 0;
    _currentComputeRootLayout_samplerRootIndex = 0;

    _currentGraphicsPipeline = nullptr;
    _currentGraphicsDescriptorSets[0] = nullptr;
    _currentGraphicsDescriptorSets[1] = nullptr;
    _currentGraphicsDescriptorSets[2] = nullptr;
    _currentGraphicsDescriptorSets[3] = nullptr;
}

void D3D12BatchBackend::endPass() {
    _commandList->OMSetRenderTargets(0, nullptr, TRUE, nullptr); // needed ?

    _currentGraphicsRootLayout_setRootIndex = 0;
    _currentGraphicsRootLayout_samplerRootIndex = 0;
    _currentComputeRootLayout_setRootIndex = 0;
    _currentComputeRootLayout_samplerRootIndex = 0;
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

void D3D12BatchBackend::clear(const FramebufferPointer& framebuffer, const core::vec4& color, float depth) {

    auto fb = static_cast<D3D12FramebufferBackend*>(framebuffer.get());
    for (UINT i = 0; i < fb->_numRenderTargets; ++i) {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv{ fb->_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + fb->_rtvDescriptorSize * i };
        _commandList->ClearRenderTargetView(rtv, color.data(), 0, nullptr);
    }

    if (fb->_dsvDescriptorHeap) {
        D3D12_CPU_DESCRIPTOR_HANDLE dsv{ fb->_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr };
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
void D3D12BatchBackend::resourceBarrierTransition(
    ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
    const BufferPointer& buffer) {
    auto d3d12bb = static_cast<D3D12BufferBackend*>(buffer.get());

    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = ResourceBarrieFlags[uint32_t(flag)];
    barrier.Transition.pResource = d3d12bb->_resource.Get();
    barrier.Transition.StateBefore = ResourceStates[uint32_t(stateBefore)];
    barrier.Transition.StateAfter = ResourceStates[uint32_t(stateAfter)];
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    _commandList->ResourceBarrier(1, &barrier);
}
void D3D12BatchBackend::resourceBarrierTransition(
    ResourceBarrierFlag flag, ResourceState stateBefore, ResourceState stateAfter,
    const TexturePointer& buffer, uint32_t subresource) {
    auto d3d12tex = static_cast<D3D12TextureBackend*>(buffer.get());

    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = ResourceBarrieFlags[uint32_t(flag)];
    barrier.Transition.pResource = d3d12tex->_resource.Get();
    barrier.Transition.StateBefore = ResourceStates[uint32_t(stateBefore)];
    barrier.Transition.StateAfter = ResourceStates[uint32_t(stateAfter)];
    barrier.Transition.Subresource = (subresource == -1 ? D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES : subresource);

    _commandList->ResourceBarrier(1, &barrier);
}

void D3D12BatchBackend::resourceBarrierRW(
    ResourceBarrierFlag flag, const TexturePointer& buffer, uint32_t subresource) {
    auto d3d12tex = static_cast<D3D12TextureBackend*>(buffer.get());

    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = ResourceBarrieFlags[uint32_t(flag)];
    barrier.UAV.pResource = d3d12tex->_resource.Get();

    _commandList->ResourceBarrier(1, &barrier);
}
void D3D12BatchBackend::resourceBarrierRW(
    ResourceBarrierFlag flag, const BufferPointer& buffer) {
    auto d3d12buf = static_cast<D3D12BufferBackend*>(buffer.get());

    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = ResourceBarrieFlags[uint32_t(flag)];
    barrier.UAV.pResource = d3d12buf->_resource.Get();

    _commandList->ResourceBarrier(1, &barrier);
}

void D3D12BatchBackend::_setViewport(const core::vec4 & viewport) {
    D3D12_VIEWPORT dxViewport;
    dxViewport.TopLeftX = viewport.x;
    dxViewport.TopLeftY = viewport.y;
    dxViewport.Width = viewport.z;
    dxViewport.Height = viewport.w;
    dxViewport.MaxDepth = 1.0f;
    dxViewport.MinDepth = 0.0f;

    _commandList->RSSetViewports(1, &dxViewport);
}

void D3D12BatchBackend::_setScissor(const core::vec4 & scissor) {
    D3D12_RECT dxRect;
    dxRect.left = (LONG) scissor.x;
    dxRect.top = (LONG) scissor.y;
    dxRect.right = (LONG) (scissor.x + scissor.z);
    dxRect.bottom = (LONG) (scissor.y + scissor.w);

    _commandList->RSSetScissorRects(1, &dxRect);
}

void D3D12BatchBackend::bindDescriptorHeap(const DescriptorHeapPointer& descriptorHeap) {
    auto dh = static_cast<D3D12DescriptorHeapBackend*>(descriptorHeap.get());

    uint32_t descriptor_heap_count = 0;
    ID3D12DescriptorHeap* descriptor_heaps[2];
    if (NULL != dh->_cbvsrvuav_heap) {
        descriptor_heaps[descriptor_heap_count] = dh->_cbvsrvuav_heap.Get();
        ++descriptor_heap_count;
    }
    if (NULL != dh->_sampler_heap) {
        descriptor_heaps[descriptor_heap_count] = dh->_sampler_heap.Get();
        ++descriptor_heap_count;
    }

    if (descriptor_heap_count > 0) {
        _commandList->SetDescriptorHeaps(descriptor_heap_count, descriptor_heaps);
    }
}

void D3D12BatchBackend::bindFramebuffer(const FramebufferPointer& framebuffer) {
    auto fbo = static_cast<D3D12FramebufferBackend*>(framebuffer.get());

    _commandList->OMSetRenderTargets(fbo->_numRenderTargets, &fbo->_rtvs, TRUE, (fbo->_dsvDescriptorSize ? &fbo->_dsv : nullptr));
}


void D3D12BatchBackend::bindRootDescriptorLayout(PipelineType type, const RootDescriptorLayoutPointer& rootDescriptorLayout) {
    auto rdl = static_cast<D3D12RootDescriptorLayoutBackend*>(rootDescriptorLayout.get());
    auto dxRS = rdl->_rootSignature;
    switch (type) {
    case PipelineType::GRAPHICS: {
        _currentGraphicsRootLayout_setRootIndex = rdl->_cbvsrvuav_rootIndex;
        _currentGraphicsRootLayout_samplerRootIndex = rdl->_sampler_rootIndex;
        _commandList->SetGraphicsRootSignature(dxRS.Get());

        _currentGraphicsDescriptorSets[0] =
            _currentGraphicsDescriptorSets[1] =
            _currentGraphicsDescriptorSets[2] =
            _currentGraphicsDescriptorSets[3] = nullptr;
    } break;
    case PipelineType::COMPUTE: {
        _currentComputeRootLayout_setRootIndex = rdl->_cbvsrvuav_rootIndex;
        _currentComputeRootLayout_samplerRootIndex = rdl->_sampler_rootIndex;
        _commandList->SetComputeRootSignature(dxRS.Get());
    } break;
    }
}

void D3D12BatchBackend::bindPipeline(const PipelineStatePointer& pipeline) {
    auto dpso = static_cast<D3D12PipelineStateBackend*>(pipeline.get());

    if (dpso->getType() == PipelineType::RAYTRACING) {
        _commandList->SetPipelineState1(dpso->_stateObject.Get());
    } else {
        if (dpso == _currentGraphicsPipeline) return; // check the cache, if the same pso then skip!

        auto dxPso = dpso->_pipelineState;
        _commandList->SetPipelineState(dxPso.Get());

        bindRootDescriptorLayout(dpso->getType(), dpso->getRootDescriptorLayout());

        if (dpso->getType() == PipelineType::GRAPHICS) {
            _commandList->IASetPrimitiveTopology(dpso->_primitive_topology);
        }
        else if (dpso->getType() == PipelineType::COMPUTE) {
        }

        _currentGraphicsPipeline = dpso;
    }
}

void D3D12BatchBackend::bindDescriptorSet(PipelineType type, const DescriptorSetPointer& descriptorSet) {
    auto dxds = static_cast<D3D12DescriptorSetBackend*>(descriptorSet.get());

    switch (type) {
    case PipelineType::GRAPHICS: {
        if (dxds->_cbvsrvuav_rootIndex >= 0) {
            if (_currentGraphicsDescriptorSets[descriptorSet->_init._bindSetSlot] == dxds)
                return;
            _commandList->SetGraphicsRootDescriptorTable(
                _currentGraphicsRootLayout_setRootIndex + descriptorSet->_init._bindSetSlot,
                dxds->_cbvsrvuav_GPUHandle);
            _currentGraphicsDescriptorSets[descriptorSet->_init._bindSetSlot] = dxds;
        }
        if (dxds->_sampler_rootIndex >= 0) {
            _commandList->SetGraphicsRootDescriptorTable(
                _currentGraphicsRootLayout_samplerRootIndex,
                dxds->_sampler_GPUHandle);
        }
    } break;
    case PipelineType::COMPUTE: {
        if (dxds->_cbvsrvuav_rootIndex >= 0) {
            _commandList->SetComputeRootDescriptorTable(
                _currentComputeRootLayout_setRootIndex + descriptorSet->_init._bindSetSlot,
                dxds->_cbvsrvuav_GPUHandle);
        }
        if (dxds->_sampler_rootIndex >= 0) {
            _commandList->SetComputeRootDescriptorTable(
                _currentComputeRootLayout_samplerRootIndex,
                dxds->_sampler_GPUHandle);
        }
    } break;
    }
}

void D3D12BatchBackend::bindPushUniform(PipelineType type, uint32_t slot, uint32_t size, const uint8_t* data) {
    switch (type) {
    case PipelineType::GRAPHICS: {
        _commandList->SetGraphicsRoot32BitConstants(slot, size >> 2, data, 0);
    } break;
    case PipelineType::COMPUTE: {
        _commandList->SetComputeRoot32BitConstants(slot, size >> 2, data, 0);
    } break;
    }
    // TODO: THe slot is actually the position of the rootsignature entry, NOT the index of the UBO slot used for it
    // Need to address that...
}


void D3D12BatchBackend::bindIndexBuffer(const BufferPointer& buffer) {
    auto dbBuffer = static_cast<D3D12BufferBackend*>(buffer.get());
    _commandList->IASetIndexBuffer(&dbBuffer->_indexBufferView);
}

void D3D12BatchBackend::bindVertexBuffers(uint32_t num, const BufferPointer * buffers) {
    auto dbBuffer = static_cast<D3D12BufferBackend*>(buffers[0].get());
    _commandList->IASetVertexBuffers(0, 1, &dbBuffer->_vertexBufferView);
}

void D3D12BatchBackend::draw(uint32_t numPrimitives, uint32_t startIndex) {
    _commandList->DrawInstanced(numPrimitives, 1, startIndex, 0);
}


void D3D12BatchBackend::drawIndexed(uint32_t numPrimitives, uint32_t startIndex) {
    _commandList->DrawIndexedInstanced(numPrimitives, 1, startIndex, 0, 0);
}

void D3D12BatchBackend::uploadTexture(const TexturePointer& dest, const UploadSubresourceLayoutArray& subresourceLayouts, const BufferPointer& src) {
    auto srcBackend = static_cast<D3D12BufferBackend*>(src.get());
    auto dstBackend = static_cast<D3D12TextureBackend*>(dest.get());

    D3D12_RESOURCE_DESC textureDesc{};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Alignment = 0;
    textureDesc.Width = dstBackend->_init.width;
    textureDesc.Height = dstBackend->_init.height;
    textureDesc.DepthOrArraySize = (dstBackend->_init.numSlices ? dstBackend->_init.numSlices : 1);
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
   

    const uint32_t numSubResources = textureDesc.MipLevels * textureDesc.DepthOrArraySize;

    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> subres_layouts(numSubResources);
    std::vector<UINT> subres_rowcounts(numSubResources, 0);
    std::vector<UINT64> subres_row_strides(numSubResources, 0);
    UINT64 buffer_size = 0;

    
    ID3D12Device* pDevice;
    _commandList->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
    pDevice->GetCopyableFootprints(&textureDesc, 0, numSubResources, 0, subres_layouts.data(), subres_rowcounts.data(), subres_row_strides.data(), &buffer_size);

    // let s check that the source buffer is big enough
    // maybe?


    // Copy buffer to texture
    // Assume a  resourceBarrier before
  //  for (uint32_t slice_num = 0; slice_num < textureDesc.DepthOrArraySize; ++slice_num) {
 //       for (uint32_t mip_level = 0; mip_level < textureDesc.MipLevels; ++mip_level) {
    for (auto& subresourceLayout : subresourceLayouts)
    {
            const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = subres_layouts[subresourceLayout.subresource];
            D3D12_TEXTURE_COPY_LOCATION srcLoc{};
            srcLoc.pResource = srcBackend->_resource.Get();
            srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            srcLoc.PlacedFootprint = subres_layouts[subresourceLayout.subresource];
            srcLoc.PlacedFootprint.Offset = subresourceLayout.byteOffset;

            D3D12_TEXTURE_COPY_LOCATION dstLoc{};
            dstLoc.pResource = dstBackend->_resource.Get();
            dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dstLoc.SubresourceIndex = subresourceLayout.subresource;

           _commandList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr );
    }
/*
    uint32_t arraySize = 1;
    uint32_t NumMips = 1;

    const uint32_t numSubResources = textureDesc.MipLevels * textureDesc.DepthOrArraySize;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts = (D3D12_PLACED_SUBRESOURCE_FOOTPRINT*)_alloca(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) * numSubResources);
    uint32_t* numRows = (uint32_t*)_alloca(sizeof(uint32_t) * numSubResources);
    uint64_t* rowSizes = (uint64_t*)_alloca(sizeof(uint64_t) * numSubResources);

    uint64_t textureMemSize = 0;
    pDevice->GetCopyableFootprints(&textureDesc, 0, uint32_t(numSubResources), 0, layouts, numRows, rowSizes, &textureMemSize);

    // Get a GPU upload buffer
    uint8_t* uploadMem = reinterpret_cast<uint8_t*>(uploadCPUMem);

    const uint8_t* srcMem = reinterpret_cast<const uint8_t*>(initData);
    const uint64_t srcTexelSize = 4; // probably ?

    for (uint32_t arrayIdx = 0; arrayIdx < texResourceDesc.DepthOrArraySize; ++arrayIdx)
    {
        uint32_t mipWidth = textureDesc.Width;
        for (uint32_t mipIdx = 0; mipIdx < textureDesc.MipLevels; ++mipIdx)
        {
            const uint32_t subResourceIdx = mipIdx + (arrayIdx * textureDesc.MipLevels);

            const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& subResourceLayout = layouts[subResourceIdx];
            const uint32_t subResourceHeight = numRows[subResourceIdx];
            const uint32_t subResourcePitch = subResourceLayout.Footprint.RowPitch;
            const uint32_t subResourceDepth = subResourceLayout.Footprint.Depth;
            const uint32_t srcPitch = mipWidth * srcTexelSize;
            uint8_t* dstSubResourceMem = uploadMem + subResourceLayout.Offset;

            for (uint32_t z = 0; z < subResourceDepth; ++z)
            {
                for (uint32_t y = 0; y < subResourceHeight; ++y)
                {
                    memcpy(dstSubResourceMem, srcMem, core::min(subResourcePitch, srcPitch));
                    dstSubResourceMem += subResourcePitch;
                    srcMem += srcPitch;
                }
            }

            mipWidth = core::max(mipWidth / 2, 1ull);
        }
    }

    for (uint32_t subResourceIdx = 0; subResourceIdx < numSubResources; ++subResourceIdx)
    {
        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = texture.Resource;
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = uint32_t(subResourceIdx);
        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = uploadResource;
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint = layouts[subResourceIdx];
        src.PlacedFootprint.Offset += resourceOffset;
        _commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    }
    // ANd one after*/
}

void D3D12BatchBackend::copyBufferRegion(const BufferPointer& dest, uint32_t destOffset, const BufferPointer& src, uint32_t srcOffset, uint32_t size) {
    auto srcBackend = static_cast<D3D12BufferBackend*>(src.get());
    auto dstBackend = static_cast<D3D12BufferBackend*>(dest.get());

    _commandList->CopyBufferRegion(dstBackend->_resource.Get(), destOffset, srcBackend->_resource.Get(), srcOffset, size);
}

void D3D12BatchBackend::uploadBuffer(const BufferPointer& dest) {
    auto dstBackend = static_cast<D3D12BufferBackend*>(dest.get());
   
    // upload the cpuDataBuffer into dest
    _commandList->CopyBufferRegion(dstBackend->_resource.Get(), 0, dstBackend->_cpuDataResource.Get(), 0, dstBackend->bufferSize());
    dstBackend->notifyUploaded();
}


void D3D12BatchBackend::dispatch(uint32_t numThreadsX, uint32_t numThreadsY, uint32_t numThreadsZ) {
    _commandList->Dispatch(numThreadsX, numThreadsY, numThreadsZ);
}

void D3D12BatchBackend::dispatchRays(const DispatchRaysArgs& args) {
    auto stb = static_cast<D3D12ShaderTableBackend*>(args.shaderTable.get());

    D3D12_DISPATCH_RAYS_DESC desc = {};
    desc.RayGenerationShaderRecord.StartAddress = args.generationShaderRecordStart + stb->_shaderTable->GetGPUVirtualAddress();
    desc.RayGenerationShaderRecord.SizeInBytes = args.generationShaderRecordSize;

    desc.MissShaderTable.StartAddress = args.missShaderRecordStart + stb->_shaderTable->GetGPUVirtualAddress();
    desc.MissShaderTable.SizeInBytes = args.missShaderRecordSize;
    desc.MissShaderTable.StrideInBytes = args.missShaderRecordStride;

    desc.HitGroupTable.StartAddress = args.hitGroupShaderRecordStart + stb->_shaderTable->GetGPUVirtualAddress();
    desc.HitGroupTable.SizeInBytes = args.hitGroupShaderRecordSize;
    desc.HitGroupTable.StrideInBytes = args.hitGroupShaderRecordStride;

    desc.CallableShaderTable.StartAddress = args.callableShaderRecordStart + stb->_shaderTable->GetGPUVirtualAddress();
    desc.CallableShaderTable.SizeInBytes = args.callableShaderRecordSize;
    desc.CallableShaderTable.StrideInBytes = args.callableShaderRecordStride;

    desc.Width = args.width;
    desc.Height = args.height;
    desc.Depth = args.depth;

    _commandList->DispatchRays(&desc);
}

#endif
