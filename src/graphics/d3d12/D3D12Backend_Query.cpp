// D3D12Backend_Query.cpp
//
// Sam Gateau - October 2021
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

#include <vector>

using namespace graphics;

#ifdef _WINDOWS
#define ThrowIfFailed(result) if (FAILED((result))) picoLog("D3D12Backend_Query FAILED !!!");

D3D12BatchTimerBackend::D3D12BatchTimerBackend() {

}

D3D12BatchTimerBackend::~D3D12BatchTimerBackend() {

}

void D3D12BatchTimerBackend::begin(ID3D12GraphicsCommandList* _commandList, INT index) {

    auto publicBuffer = static_cast<D3D12BufferBackend*>(_buffer.get());
    auto publicBufferResource = publicBuffer->_resource.Get();
    D3D12_RESOURCE_BARRIER publicResource_barrier;
    publicResource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    publicResource_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    publicResource_barrier.Transition.pResource = publicBufferResource;
    publicResource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    publicResource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    publicResource_barrier.Transition.Subresource = 0;
    _commandList->ResourceBarrier(1, &publicResource_barrier);


  //  auto prevFrameIndex = ((index + 2) % 3);

    index = _currentSampleIndex;
    auto prevFrameIndex = ((index + (_init.numSamples - 1)) % _init.numSamples);

    auto destBuffer = _queryResult.Get();
    // auto destBuffer = _queryResultMapped.Get();
       // resolve data for previous frame
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = destBuffer;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = 0;
    _commandList->ResourceBarrier(1, &barrier);

    _commandList->ResolveQueryData(_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, prevFrameIndex * 2, 2, destBuffer, prevFrameIndex * 2 * 8);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    _commandList->ResourceBarrier(1, &barrier);


    auto mappedBuffer = _queryResultMapped.Get();
    _commandList->CopyResource(mappedBuffer, destBuffer);

    _commandList->CopyResource(publicBufferResource, destBuffer);


    publicResource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    publicResource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    _commandList->ResourceBarrier(1, &publicResource_barrier);

    _commandList->EndQuery(_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, index * 2);
}

void D3D12BatchTimerBackend::end(ID3D12GraphicsCommandList* _commandList, INT index) {
    index = _currentSampleIndex;

    // timestamp end of batch
    _commandList->EndQuery(_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, index * 2 + 1);

    _currentSampleIndex = (_currentSampleIndex + 1) % _init.numSamples;


    // grab last timing recording
   // auto prevFrameIndex = ((index + 2) % 3);
    auto prevFrameIndex = ((index + (_init.numSamples - 1)) % _init.numSamples);
    uint64_t* timingResults = (uint64_t*)_queryResultCPUMappedAddress;


    auto gpuFrameTiming = (timingResults[prevFrameIndex * 2 + 1] - timingResults[prevFrameIndex * 2]) / _commandQueueTimestampFrequency;
  //  picoLog(std::to_string(gpuFrameTiming));
}

BatchTimerPointer D3D12Backend::createBatchTimer(const BatchTimerInit& init) {
	auto timer = std::make_shared<D3D12BatchTimerBackend>();

    timer->_init = init;

    // Describe and create a heap for occlusion queries.
    D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
    queryHeapDesc.Count = D3D12Backend::CHAIN_NUM_FRAMES * 2;
    queryHeapDesc.Count = init.numSamples * 2;

    queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    ThrowIfFailed(_device->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&timer->_queryHeap)));

    auto numBlocks = ((uint32_t)8 * 2 * init.numSamples) / 256;
    int bufferSize = (numBlocks + 1) * 256;

    BufferInit bi;
    bi.bufferSize = 8 * 2 * init.numSamples;
    bi.usage = ResourceUsage::RESOURCE_BUFFER;
    bi.structStride =  8 * 2;
    bi.numElements = init.numSamples;
  //  bi.raw = true;
    timer->_buffer = createBuffer(bi);

 //   auto d3d12bb = static_cast<D3D12BufferBackend*>(timer->_buffer.get());
 //   timer->_queryResult = (d3d12bb->_resource);

    D3D12_HEAP_PROPERTIES heapProp;
    heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
    //heapProp.Type = D3D12_HEAP_TYPE_READBACK;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProp.CreationNodeMask = 1;
    heapProp.VisibleNodeMask = 1;
    D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;


    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = bufferSize;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ThrowIfFailed(_device->CreateCommittedResource(&heapProp,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        nullptr,
        IID_PPV_ARGS(&timer->_queryResult)
    ));

    // Same buffer but mapped
    heapProp.Type = D3D12_HEAP_TYPE_READBACK;
    ThrowIfFailed(_device->CreateCommittedResource(&heapProp,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&timer->_queryResultMapped)
    ));

    D3D12_RANGE read_range = { 0, 0 };
    timer->_queryResultMapped->Map(0, &read_range, (void**)&(timer->_queryResultCPUMappedAddress));

    timer->_commandQueueTimestampFrequency = _commandQueueTimestampFrequency;

	return timer;
}

#endif