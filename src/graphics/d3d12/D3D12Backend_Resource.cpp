// D3D12Backend_Resource.cpp
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

using namespace graphics;

#ifdef _WINDOWS

D3D12BufferBackend* CreateBuffer(D3D12Backend* backend, const BufferInit& init) {
    uint64_t bufferSize = init.bufferSize;
    // Align the buffer size to multiples of 256
   if (init.usage && ResourceUsage::UNIFORM_BUFFER) {
        auto numBlocks = ((uint32_t) init.bufferSize) / 256;
        auto blockModulo = ((uint32_t)init.bufferSize) % 256;
        bufferSize = (numBlocks + (blockModulo > 0)) * 256;

        if (init.swapchainable) {
            bufferSize *= D3D12Backend::CHAIN_NUM_FRAMES;
        }
    }

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
    if (init.usage & ResourceUsage::RW_RESOURCE_BUFFER) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    // Adjust for padding
    UINT64 padded_size = 0;
    backend->_device->GetCopyableFootprints(&desc, 0, 1, 0, NULL, NULL, NULL, &padded_size);
    bufferSize = (uint64_t)padded_size;
    desc.Width = padded_size;

    D3D12_RESOURCE_STATES res_states = D3D12_RESOURCE_STATE_COPY_DEST;
    if (init.usage & ResourceUsage::VERTEX_BUFFER) {
        res_states = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }

    if (init.usage & ResourceUsage::INDEX_BUFFER) {
        res_states = D3D12_RESOURCE_STATE_INDEX_BUFFER;
    }

    if (init.usage & ResourceUsage::UNIFORM_BUFFER) {
        res_states = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }

    if (init.usage & ResourceUsage::RESOURCE_BUFFER) {
        res_states = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        res_states |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }

    if (init.usage & ResourceUsage::RW_RESOURCE_BUFFER) {
        res_states = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    // Host visible is ok if not a RW_resouce
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
    bufferBackend->_resource = dxResource;
    bufferBackend->_bufferSize = bufferSize;


    if (init.hostVisible) {
        D3D12_RANGE read_range = { 0, 0 };
        hres = dxResource->Map(0, &read_range, (void**)&(bufferBackend->_cpuMappedAddress));
    }

    if (init.usage & ResourceUsage::VERTEX_BUFFER) {
        bufferBackend->_vertexBufferView.BufferLocation = dxResource->GetGPUVirtualAddress();
        bufferBackend->_vertexBufferView.SizeInBytes = (UINT)bufferSize;
        bufferBackend->_vertexBufferView.StrideInBytes = init.vertexStride;
        // Format is filled out by tr_create_vertex_buffer
    }

    if (init.usage & ResourceUsage::UNIFORM_BUFFER) {
        bufferBackend->_uniformBufferView.BufferLocation = dxResource->GetGPUVirtualAddress();
        bufferBackend->_uniformBufferView.SizeInBytes = (UINT)bufferSize;
    }

    if (init.usage & ResourceUsage::INDEX_BUFFER) {
        bufferBackend->_indexBufferView.BufferLocation = dxResource->GetGPUVirtualAddress();
        bufferBackend->_indexBufferView.SizeInBytes = (UINT)bufferSize;
        bufferBackend->_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    }

    if (init.usage & ResourceUsage::RESOURCE_BUFFER) {
        bufferBackend->_resourceBufferView.Format = DXGI_FORMAT_UNKNOWN;
        bufferBackend->_resourceBufferView.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        bufferBackend->_resourceBufferView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        bufferBackend->_resourceBufferView.Buffer.FirstElement = bufferBackend->_init.firstElement;
        bufferBackend->_resourceBufferView.Buffer.NumElements = (UINT)(bufferBackend->_init.numElements);
        bufferBackend->_resourceBufferView.Buffer.StructureByteStride = (UINT)(bufferBackend->_init.structStride);
        bufferBackend->_resourceBufferView.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        if (bufferBackend->_init.raw) {
            bufferBackend->_resourceBufferView.Buffer.StructureByteStride = 0;
            bufferBackend->_resourceBufferView.Format = DXGI_FORMAT_R32_TYPELESS;
            bufferBackend->_resourceBufferView.Buffer.Flags |= D3D12_BUFFER_SRV_FLAG_RAW;
        }
    }

    if (init.usage & ResourceUsage::RW_RESOURCE_BUFFER) {
        bufferBackend->_rwResourceBufferView.Format = DXGI_FORMAT_UNKNOWN;
        bufferBackend->_rwResourceBufferView.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        bufferBackend->_rwResourceBufferView.Buffer.FirstElement = bufferBackend->_init.firstElement;
        bufferBackend->_rwResourceBufferView.Buffer.NumElements = (UINT)(bufferBackend->_init.numElements);
        bufferBackend->_rwResourceBufferView.Buffer.StructureByteStride = (UINT)(bufferBackend->_init.structStride);
        bufferBackend->_rwResourceBufferView.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        bufferBackend->_rwResourceBufferView.Buffer.CounterOffsetInBytes = 0;
        if (bufferBackend->_init.raw) {
            bufferBackend->_rwResourceBufferView.Buffer.StructureByteStride = 0;
            bufferBackend->_rwResourceBufferView.Format = DXGI_FORMAT_R32_TYPELESS;
            bufferBackend->_rwResourceBufferView.Buffer.Flags |= D3D12_BUFFER_UAV_FLAG_RAW;
        }
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






D3D12TextureBackend* CreateTexture(D3D12Backend* backend, const TextureInit& init) {

    // TODO: Should get all that from the init
   // auto d3d12Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    auto d3d12Format = D3D12Backend::Format[(uint32_t) init.format];
    auto texDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;


    D3D12_HEAP_PROPERTIES heap_props {};
    heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_props.CreationNodeMask = 1;
    heap_props.VisibleNodeMask = 1;

    D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE;

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = texDimension;
    desc.Alignment = 0;
    desc.Width = init.width;
    desc.Height = init.height;
    desc.DepthOrArraySize = (init.numSlices ? init.numSlices : 1); // if init numSLices is 0 then just 1, else it s an array
    desc.MipLevels = (UINT16)1;
    desc.Format = d3d12Format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    if (init.usage & ResourceUsage::RENDER_TARGET) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
 /*   if (p_texture->usage & tr_texture_usage_depth_stencil_attachment) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }*/
    if (init.usage & ResourceUsage::RW_RESOURCE_TEXTURE) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    // Default resource state is ready to be read from
    D3D12_RESOURCE_STATES res_states = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  /* // We could do that but we are not, it s just easier to assume read only and do transition i think at the moment...
    if (init.usage & ResourceUsage::RENDER_TARGET) {
        // or render target if it s the usage
        res_states = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    if (init.usage & ResourceUsage::RW_RESOURCE_TEXTURE) {
        // or rw aka uav
        res_states = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }*/

    D3D12_CLEAR_VALUE clear_value{};
    clear_value.Format = d3d12Format;
  /*  if (tr_texture_usage_depth_stencil_attachment == (p_texture->usage & tr_texture_usage_depth_stencil_attachment)) {
        clear_value.DepthStencil.Depth = p_texture->clear_value.depth;
        clear_value.DepthStencil.Stencil = p_texture->clear_value.stencil;
    }
    else {
        clear_value.Color[0] = p_texture->clear_value.r;
        clear_value.Color[1] = p_texture->clear_value.g;
        clear_value.Color[2] = p_texture->clear_value.b;
        clear_value.Color[3] = p_texture->clear_value.a;
    }*/
    clear_value.Color[0] = 0.0f;
    clear_value.Color[1] = 1.0f;
    clear_value.Color[2] = 0.0f;
    clear_value.Color[3] = 1.0f;

    D3D12_CLEAR_VALUE* p_clear_value = NULL;
    if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) || (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) {
        p_clear_value = &clear_value;
    }

    auto d3d12TextureBackend = new D3D12TextureBackend();
    d3d12TextureBackend->_init = init;
    d3d12TextureBackend->_resource;

    // No need to upload if used as render target or RW resource
    if (init.usage & (ResourceUsage::RENDER_TARGET | ResourceUsage::RW_RESOURCE_TEXTURE) ) {
        d3d12TextureBackend->notifyUploaded(); 
    }

    HRESULT hres = backend->_device->CreateCommittedResource(
        &heap_props, heap_flags, &desc, res_states, p_clear_value,
        __uuidof(d3d12TextureBackend->_resource), (void**)&(d3d12TextureBackend->_resource));
   // assert(SUCCEEDED(hres));

    d3d12TextureBackend->_shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    d3d12TextureBackend->_shaderResourceViewDesc.Format = d3d12Format;
    
    if (init.numSlices > 0) {
        d3d12TextureBackend->_shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        d3d12TextureBackend->_shaderResourceViewDesc.Texture2DArray.ArraySize = init.numSlices;
        d3d12TextureBackend->_shaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;
        d3d12TextureBackend->_shaderResourceViewDesc.Texture2DArray.MipLevels = 1;
        d3d12TextureBackend->_shaderResourceViewDesc.Texture2DArray.PlaneSlice = 0;
        d3d12TextureBackend->_shaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0;
        d3d12TextureBackend->_shaderResourceViewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
    } else {
        d3d12TextureBackend->_shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        d3d12TextureBackend->_shaderResourceViewDesc.Texture2D.MipLevels = 1;
        d3d12TextureBackend->_shaderResourceViewDesc.Texture2D.PlaneSlice = 0;
        d3d12TextureBackend->_shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
        d3d12TextureBackend->_shaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    }

    if (init.usage & ResourceUsage::RW_RESOURCE_TEXTURE) {
        
        d3d12TextureBackend->_unorderedAccessViewDesc.Format = d3d12Format;
        
        if (init.numSlices > 0) {
            d3d12TextureBackend->_unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            d3d12TextureBackend->_unorderedAccessViewDesc.Texture2DArray.ArraySize = init.numSlices;
            d3d12TextureBackend->_unorderedAccessViewDesc.Texture2DArray.FirstArraySlice = 0;
            d3d12TextureBackend->_unorderedAccessViewDesc.Texture2DArray.MipSlice = 0;
            d3d12TextureBackend->_unorderedAccessViewDesc.Texture2DArray.PlaneSlice = 0;
        }
        else {
            d3d12TextureBackend->_unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            d3d12TextureBackend->_unorderedAccessViewDesc.Texture2D.MipSlice = 0;
            d3d12TextureBackend->_unorderedAccessViewDesc.Texture2D.PlaneSlice = 0;
        }

    }

    return d3d12TextureBackend;
}










D3D12TextureBackend::D3D12TextureBackend() {

}
D3D12TextureBackend::~D3D12TextureBackend() {

}
TexturePointer D3D12Backend::createTexture(const TextureInit& init) {
    auto texture = CreateTexture(this, init);

    return TexturePointer(texture);
}





D3D12GeometryBackend::D3D12GeometryBackend() {

}

D3D12GeometryBackend::~D3D12GeometryBackend() {

}

GeometryPointer D3D12Backend::createGeometry(const GeometryInit& init) {
    auto geometry = std::make_shared<D3D12GeometryBackend>();

    geometry->_init = init;

    return geometry;
}

#endif
