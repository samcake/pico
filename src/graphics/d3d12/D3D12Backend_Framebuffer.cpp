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

        rtv.Format = D3D12Backend::Format[(uint32_t)tex->format()];

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
        framebuffer->_numColorBuffers = 1;

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = numColorRTs;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;
        D3D12Backend_Check(_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));
        framebuffer->_rtvDescriptorHeap = descriptorHeap;
        framebuffer->_rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        framebuffer->_rtvs = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

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
            framebuffer->_colorBuffers.push_back(tex);
        }
    }

    if (numDepthStencil) {
        framebuffer->_numDepthBuffers = 1;

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = 1;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;
        D3D12Backend_Check(_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));
        framebuffer->_dsvDescriptorHeap = descriptorHeap;
        framebuffer->_dsvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        framebuffer->_dsv = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

        auto tex = init.depthStencilTarget;
        if (tex->_init.usage & ResourceUsage::RENDER_TARGET) {
            auto d3d12TextureBackend = static_cast<D3D12TextureBackend*> (tex.get());

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvdesc;
            if (textureToDSV(d3d12TextureBackend, dsvdesc)) {
                _device->CreateDepthStencilView(d3d12TextureBackend->_resource.Get(), &dsvdesc, framebuffer->_dsv);
            }

            width = std::min(width, tex->width());
            height = std::min(height, tex->height());
        }
        framebuffer->_depthBuffers.push_back(tex);
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

FramebufferPointer D3D12Backend::createFramebuffer(const FramebufferInit_Swapable& init) {
    auto framebuffer = new D3D12FramebufferBackend();
    framebuffer->_chainLength = init.chainLength;
    framebuffer->_numRenderTargets = 1;
    framebuffer->_numColorBuffers = init.chainLength;

    // RTV heap: N slots (one per buffer)
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = init.chainLength;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    D3D12Backend_Check(_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
    framebuffer->_rtvDescriptorHeap = rtvHeap;
    framebuffer->_rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    framebuffer->_rtvs = rtvHeap->GetCPUDescriptorHandleForHeapStart();

    auto rtvHandle = framebuffer->_rtvs;
    for (uint32_t i = 0; i < init.chainLength; ++i) {
        TextureInit colorInit;
        colorInit.usage = ResourceUsage::RENDER_TARGET;
        colorInit.width = init.width;
        colorInit.height = init.height;
        colorInit.format = init.colorFormat;
        auto colorTex = createTexture(colorInit);
        framebuffer->_colorBuffers.push_back(colorTex);

        auto d3d12Tex = static_cast<D3D12TextureBackend*>(colorTex.get());
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = Format[(uint32_t)init.colorFormat];
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        _device->CreateRenderTargetView(d3d12Tex->_resource.Get(), &rtvDesc, rtvHandle);
        rtvHandle.ptr += framebuffer->_rtvDescriptorSize;
    }

    if (init.depthBuffer) {
        framebuffer->_numDepthBuffers = init.chainLength;

        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = init.chainLength;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        ComPtr<ID3D12DescriptorHeap> dsvHeap;
        D3D12Backend_Check(_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));
        framebuffer->_dsvDescriptorHeap = dsvHeap;
        framebuffer->_dsvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        framebuffer->_dsv = dsvHeap->GetCPUDescriptorHandleForHeapStart();

        auto dsvHandle = framebuffer->_dsv;
        for (uint32_t i = 0; i < init.chainLength; ++i) {
            TextureInit depthInit;
            depthInit.usage = ResourceUsage::RENDER_TARGET;
            depthInit.width = init.width;
            depthInit.height = init.height;
            depthInit.format = init.depthFormat;
            auto depthTex = createTexture(depthInit);
            framebuffer->_depthBuffers.push_back(depthTex);

            auto d3d12Tex = static_cast<D3D12TextureBackend*>(depthTex.get());
            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
            _device->CreateDepthStencilView(d3d12Tex->_resource.Get(), &dsvDesc, dsvHandle);
            dsvHandle.ptr += framebuffer->_dsvDescriptorSize;
        }
    }

    framebuffer->_init.width = init.width;
    framebuffer->_init.height = init.height;

    return FramebufferPointer(framebuffer);
}

// Private: wraps existing swapchain RTVs/DSVs into a Framebuffer without allocating new textures.
FramebufferPointer D3D12Backend::createFramebufferFromSwapchain(D3D12SwapchainBackend* sw) {
    auto framebuffer = new D3D12FramebufferBackend();
    framebuffer->_chainLength = CHAIN_NUM_FRAMES;
    framebuffer->_numRenderTargets = 1;
    framebuffer->_numColorBuffers = CHAIN_NUM_FRAMES;

    // Share the swapchain's existing RTV heap directly
    framebuffer->_rtvDescriptorHeap = sw->_rtvDescriptorHeap;
    framebuffer->_rtvDescriptorSize = sw->_rtvDescriptorSize;
    framebuffer->_rtvs = sw->_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    if (sw->_dsvDescriptorHeap) {
        // Swapchain depth is a single shared resource — 1 DSV slot, index always 0
        framebuffer->_numDepthBuffers = 1;
        framebuffer->_dsvDescriptorHeap = sw->_dsvDescriptorHeap;
        framebuffer->_dsvDescriptorSize = sw->_dsvDescriptorSize;
        framebuffer->_dsv = sw->_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    }

    framebuffer->_init.width = sw->width();
    framebuffer->_init.height = sw->height();
    framebuffer->_currentIndex = sw->_currentIndex;

    return FramebufferPointer(framebuffer);
}

void D3D12Backend::rebuildFramebufferHeaps(D3D12FramebufferBackend* fb) {
    auto rtvHandle = fb->_rtvs;
    for (auto& tex : fb->_colorBuffers) {
        auto d3d12Tex = static_cast<D3D12TextureBackend*>(tex.get());
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = Format[(uint32_t)tex->format()];
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        _device->CreateRenderTargetView(d3d12Tex->_resource.Get(), &rtvDesc, rtvHandle);
        rtvHandle.ptr += fb->_rtvDescriptorSize;
    }

    if (fb->_dsvDescriptorHeap && !fb->_depthBuffers.empty()) {
        auto dsvHandle = fb->_dsv;
        for (auto& tex : fb->_depthBuffers) {
            auto d3d12Tex = static_cast<D3D12TextureBackend*>(tex.get());
            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
            _device->CreateDepthStencilView(d3d12Tex->_resource.Get(), &dsvDesc, dsvHandle);
            dsvHandle.ptr += fb->_dsvDescriptorSize;
        }
    }
}

void D3D12Backend::resizeTexture(const TexturePointer& texture, uint32_t width, uint32_t height) {
    if (texture->width() == width && texture->height() == height) return;

    auto d3d12Tex = static_cast<D3D12TextureBackend*>(texture.get());
    d3d12Tex->_init.width = width;
    d3d12Tex->_init.height = height;

    auto newTex = createTexture(d3d12Tex->_init);
    d3d12Tex->_resource = static_cast<D3D12TextureBackend*>(newTex.get())->_resource;
}

void D3D12Backend::resizeFramebuffer(const FramebufferPointer& framebuffer, uint32_t width, uint32_t height) {
    if (framebuffer->width() == width && framebuffer->height() == height) return;

    flush();

    auto fb = static_cast<D3D12FramebufferBackend*>(framebuffer.get());

    for (auto& tex : fb->_colorBuffers) resizeTexture(tex, width, height);
    for (auto& tex : fb->_depthBuffers) resizeTexture(tex, width, height);

    rebuildFramebufferHeaps(fb);

    fb->_init.width = width;
    fb->_init.height = height;
    fb->_currentIndex = 0;
}

#endif