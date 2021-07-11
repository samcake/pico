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

#include <vector>

using namespace graphics;

#ifdef _WINDOWS

bool textureToRTV(D3D12TextureBackend* tex, D3D12_RENDER_TARGET_VIEW_DESC& rtv) {

    if (tex->_init.usage & ResourceUsage::RENDER_TARGET) {
        auto d3d12Format = DXGI_FORMAT_R8G8B8A8_UNORM;

        rtv.Format = d3d12Format;

        if (tex->_init.numSlices > 0) {
            rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtv.Texture2DArray.ArraySize = tex->_init.numSlices;
            rtv.Texture2DArray.FirstArraySlice = 0;
            rtv.Texture2DArray.MipSlice = 0;
            rtv.Texture2DArray.PlaneSlice = 0;
        }
        else {
            rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtv.Texture2D.MipSlice = 0;
            rtv.Texture2D.PlaneSlice = 0;
        }
    
        return true;
    }

    return false;
}

bool textureToDSV(D3D12TextureBackend* tex, D3D12_DEPTH_STENCIL_VIEW_DESC& dsv) {

    if (tex->_init.usage & ResourceUsage::RENDER_TARGET) {
        auto d3d12Format = DXGI_FORMAT_D32_FLOAT;

        dsv.Format = d3d12Format;
        dsv.Flags = D3D12_DSV_FLAG_NONE;

        if (tex->_init.numSlices > 0) {
            dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            dsv.Texture2DArray.ArraySize = tex->_init.numSlices;
            dsv.Texture2DArray.FirstArraySlice = 0;
            dsv.Texture2DArray.MipSlice = 0;
        }
        else {
            dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsv.Texture2D.MipSlice = 0;
        }

        return true;
    }

    return false;
}


D3D12FramebufferBackend::D3D12FramebufferBackend() : Framebuffer() {

}

D3D12FramebufferBackend::~D3D12FramebufferBackend() {

}

FramebufferPointer D3D12Backend::createFramebuffer(const FramebufferInit& init) {
    auto framebuffer = new D3D12FramebufferBackend();

    auto numColorRTs = std::min((uint32_t)init.colorTargets.size(), FRAMEBUFFER_MAX_NUM_COLOR_TARGETS);
    auto numDepthStencil = (init.depthStencilTarget ? 1 : 0);
    
    auto width = (init.width == 0 ? 0xFFFFFFFF : init.width);
    auto height = (init.height == 0 ? 0xFFFFFFFF : init.height);

    if (numColorRTs) {
        framebuffer->_numRenderTargets = numColorRTs;

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = numColorRTs;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;
        _device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));
        framebuffer->_rtvDescriptorHeap = descriptorHeap;
        framebuffer->_rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        framebuffer->_rtvs = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

        auto d3d12Format = DXGI_FORMAT_R8G8B8A8_UNORM;

        auto rtvHandle = framebuffer->_rtvs;

        for (uint32_t i = 0; i < numColorRTs; i++) {
            auto tex = init.colorTargets[i];
            if (tex->_init.usage & ResourceUsage::RENDER_TARGET) {
                auto d3d12TextureBackend = static_cast<D3D12TextureBackend*> (tex.get());

                D3D12_RENDER_TARGET_VIEW_DESC rtvdesc;
                if (textureToRTV(d3d12TextureBackend, rtvdesc)) {
                    _device->CreateRenderTargetView(d3d12TextureBackend->_resource.Get(), &rtvdesc, rtvHandle);
                }
            
                width = std::min(width, tex->width());
                height = std::min(height, tex->height());

                rtvHandle.ptr += framebuffer->_rtvDescriptorSize;
            }
        }
    }

    if (numDepthStencil) {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = numDepthStencil;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;
        _device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));
        framebuffer->_dsvDescriptorHeap = descriptorHeap;
        framebuffer->_dsvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        framebuffer->_dsv = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

        auto dsvHandle = framebuffer->_dsv;

        auto tex = init.depthStencilTarget;
        if (tex->_init.usage & ResourceUsage::RENDER_TARGET) {
            auto d3d12TextureBackend = static_cast<D3D12TextureBackend*> (tex.get());

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvdesc;
            if (textureToDSV(d3d12TextureBackend, dsvdesc)) {
                _device->CreateDepthStencilView(d3d12TextureBackend->_resource.Get(), &dsvdesc, dsvHandle);
            }

            width = std::min(width, tex->width());
            height = std::min(height, tex->height());
        }
    }
    
    framebuffer->_init = init;

    if (init.width == 0) {
        framebuffer->_init.width = width;
    }

    if (init.height == 0) {
        framebuffer->_init.height = height;
    }

    return FramebufferPointer(framebuffer);
}

#endif