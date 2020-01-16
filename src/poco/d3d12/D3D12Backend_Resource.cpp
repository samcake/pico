// D3D12Backend_Resource.cpp
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

using namespace poco;

#ifdef WIN32

D3D12BufferBackend* CreateBuffer(D3D12Backend* backend, const BufferInit& init) {
    uint64_t bufferSize = init.bufferSize;
    // Align the buffer size to multiples of 256
 /*   if (p_buffer->usage & tr_buffer_usage_uniform_cbv) {
        p_buffer->size = tr_round_up((uint32_t)(init.bufferSize), 256);
    }*/

    D3D12_RESOURCE_DIMENSION res_dim = D3D12_RESOURCE_DIMENSION_BUFFER;
    D3D12_HEAP_PROPERTIES heapProp;
    heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProp.CreationNodeMask = 1;
    heapProp.VisibleNodeMask = 1;

    D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;

    D3D12_RESOURCE_DESC desc;
    desc.Dimension = res_dim;
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
    //if ((init.usage & ResourceState::) || (p_buffer->usage & tr_buffer_usage_counter_uav)) {
    //    desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    //}

    // Adjust for padding
    UINT64 padded_size = 0;
    backend->_device->GetCopyableFootprints(&desc, 0, 1, 0, NULL, NULL, NULL, &padded_size);
    bufferSize = (uint64_t)padded_size;
    desc.Width = padded_size;

    D3D12_RESOURCE_STATES res_states = D3D12_RESOURCE_STATE_COPY_DEST;
    switch (init.usage) {
    case ResourceState::VERTEX_AND_CONSTANT_BUFFER: {
        res_states = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }
    break;
    case ResourceState::INDEX_BUFFER: {
        res_states = D3D12_RESOURCE_STATE_INDEX_BUFFER;
    }
    break;
    case ResourceState::UNORDERED_ACCESS: {
        res_states = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    break;
    }

    if (init.hostVisible) {
        // D3D12_HEAP_TYPE_UPLOAD requires D3D12_RESOURCE_STATE_GENERIC_READ
        heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
        res_states = D3D12_RESOURCE_STATE_GENERIC_READ;
    }



    ID3D12Resource* dxResource = nullptr;
    HRESULT hres = backend->_device->CreateCommittedResource(
        &heapProp, heapFlags, &desc, res_states, NULL,
        __uuidof(dxResource), (void**)&(dxResource));


    D3D12BufferBackend* bufferBackend = new D3D12BufferBackend();

    bufferBackend->_init = init;
    bufferBackend->_buffer = dxResource;
    bufferBackend->_bufferSize = bufferSize;


    if (init.hostVisible) {
        D3D12_RANGE read_range = { 0, 0 };
        hres = dxResource->Map(0, &read_range, (void**)&(bufferBackend->_cpuMappedAddress));
    }

    switch (init.usage) {
    case ResourceState::VERTEX_AND_CONSTANT_BUFFER: {
        bufferBackend->_vertexBufferView.BufferLocation = dxResource->GetGPUVirtualAddress();
        bufferBackend->_vertexBufferView.SizeInBytes = (UINT)bufferSize;
        bufferBackend->_vertexBufferView.StrideInBytes = init.vertexStride;
        // Format is filled out by tr_create_vertex_buffer
    }
    break;

    case ResourceState::INDEX_BUFFER: {
        bufferBackend->_indexBufferView.BufferLocation = dxResource->GetGPUVirtualAddress();
        bufferBackend->_indexBufferView.SizeInBytes = (UINT)bufferSize;
        bufferBackend->_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    }
    break;
    }

    return bufferBackend;

}

D3D12BufferBackend::D3D12BufferBackend() {

}

D3D12BufferBackend::~D3D12BufferBackend() {

}

BufferPointer D3D12Backend::createBuffer(const BufferInit& init) {
    auto buffer = CreateBuffer(this, init);

    return BufferPointer(buffer);
}

#endif
