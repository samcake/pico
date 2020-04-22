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

using namespace pico;

#ifdef WIN32

D3D12BufferBackend* CreateBuffer(D3D12Backend* backend, const BufferInit& init) {
    uint64_t bufferSize = init.bufferSize;
    // Align the buffer size to multiples of 256
   if (init.usage == ResourceUsage::UNIFORM_BUFFER) {
        auto numBlocks = ((uint32_t) init.bufferSize) / 256;

        bufferSize = (numBlocks + 1) * 256;

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
    case ResourceUsage::VERTEX_BUFFER: {
        res_states = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }
    break;
    case ResourceUsage::INDEX_BUFFER: {
        res_states = D3D12_RESOURCE_STATE_INDEX_BUFFER;
    }
    break;
    case ResourceUsage::UNIFORM_BUFFER: {
        res_states = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }
    break;
  /*  case ResourceUsage::UNORDERED_ACCESS: {
        res_states = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    break;
    */}

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

    switch (init.usage) {
    case ResourceUsage::VERTEX_BUFFER: {
        bufferBackend->_vertexBufferView.BufferLocation = dxResource->GetGPUVirtualAddress();
        bufferBackend->_vertexBufferView.SizeInBytes = (UINT)bufferSize;
        bufferBackend->_vertexBufferView.StrideInBytes = init.vertexStride;
        // Format is filled out by tr_create_vertex_buffer
    }
    break;

    case ResourceUsage::UNIFORM_BUFFER: {
        bufferBackend->_uniformBufferView.BufferLocation = dxResource->GetGPUVirtualAddress();
        bufferBackend->_uniformBufferView.SizeInBytes = (UINT)bufferSize;
    }
    break;

    case ResourceUsage::INDEX_BUFFER: {
        bufferBackend->_indexBufferView.BufferLocation = dxResource->GetGPUVirtualAddress();
        bufferBackend->_indexBufferView.SizeInBytes = (UINT)bufferSize;
        bufferBackend->_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    }
    break;

    case ResourceUsage::RESOURCE_BUFFER: {
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












D3D12TextureBackend* CreateTexture(D3D12Backend* backend, const TextureInit& init) {

    // TODO: Should get all that from the init
   // auto d3d12Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    auto d3d12Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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
    desc.DepthOrArraySize = 1;
    desc.MipLevels = (UINT16)1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //d3d12Format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
      /*  if (p_texture->usage & tr_texture_usage_color_attachment) {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }
        if (p_texture->usage & tr_texture_usage_depth_stencil_attachment) {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }
        if (p_texture->usage & tr_texture_usage_storage_image) {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }
*/
        D3D12_RESOURCE_STATES res_states = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    /*    if (p_texture->usage & tr_texture_usage_color_attachment) {
            res_states = D3D12_RESOURCE_STATE_RENDER_TARGET;
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
    clear_value.Color[1] = 0.0f;
    clear_value.Color[2] = 0.0f;
    clear_value.Color[3] = 1.0f;

    D3D12_CLEAR_VALUE* p_clear_value = NULL;
   /* if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) || (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) {
        p_clear_value = &clear_value;
    }*/

    auto d3d12TextureBackend = new D3D12TextureBackend();
    d3d12TextureBackend->_init = init;
    d3d12TextureBackend->_resource;

    HRESULT hres = backend->_device->CreateCommittedResource(
        &heap_props, heap_flags, &desc, res_states, p_clear_value,
        __uuidof(d3d12TextureBackend->_resource), (void**)&(d3d12TextureBackend->_resource));
   // assert(SUCCEEDED(hres));

  //  p_texture->owns_image = true;
   // }

 //   if (p_texture->usage & tr_texture_usage_sampled_image) {
      /*  D3D12_SRV_DIMENSION view_dim = D3D12_SRV_DIMENSION_UNKNOWN;
        switch (p_texture->type) {
        case tr_texture_type_1d: view_dim = D3D12_SRV_DIMENSION_TEXTURE1D; break;
        case tr_texture_type_2d: view_dim = D3D12_SRV_DIMENSION_TEXTURE2D; break;
        case tr_texture_type_3d: view_dim = D3D12_SRV_DIMENSION_TEXTURE3D; break;
        case tr_texture_type_cube: view_dim = D3D12_SRV_DIMENSION_TEXTURE2D; break;
        }
        assert(D3D12_SRV_DIMENSION_UNKNOWN != view_dim);
*/
      auto view_dim = D3D12_SRV_DIMENSION_TEXTURE2D;

      d3d12TextureBackend->_shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      d3d12TextureBackend->_shaderResourceViewDesc.Format = d3d12Format;
      d3d12TextureBackend->_shaderResourceViewDesc.ViewDimension = view_dim;
      d3d12TextureBackend->_shaderResourceViewDesc.Texture2D.MipLevels = 1;
      d3d12TextureBackend->_shaderResourceViewDesc.Texture2D.PlaneSlice = 0;
      d3d12TextureBackend->_shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
      d3d12TextureBackend->_shaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
  //  }
/*
    if (p_texture->usage & tr_texture_usage_storage_image) {
        p_texture->dx_uav_view_desc.Format = tr_util_to_dx_format(p_texture->format);
        p_texture->dx_uav_view_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        p_texture->dx_uav_view_desc.Texture2D.MipSlice = 0;
        p_texture->dx_uav_view_desc.Texture2D.PlaneSlice = 0;
    }

*/

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


#endif
