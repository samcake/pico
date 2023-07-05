// Imgui.cpp
// Encapsulation of Dear Imgui for pico
// 
// Dear ImGui by Omar Cornut: Bloat-free Graphical User interface for C++ with minimal dependencies
// https://github.com/ocornut/imgui
// 
// Sam Gateau - July 2021
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
#include "Imgui.h"

#include <graphics/render/Renderer.h>

#ifdef WIN32

#define ImTextureID D3D12_GPU_DESCRIPTOR_HANDLE *

#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

#include <graphics/d3d12/D3D12Backend.h>

using namespace uix;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static ID3D12DescriptorHeap* g_pd3dSrvDescHeap;

void Imgui::create() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //  io.Fonts->AddFontDefault();
}

void Imgui::destroy() {
    if (g_pd3dSrvDescHeap) {
        g_pd3dSrvDescHeap->Release();
    }
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Imgui::setup(const WindowPointer& win, const graphics::DevicePointer& gpudevice, graphics::PixelFormat colorTargetFormat) {

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(win->nativeWindow());
    auto d3d12Device = static_cast<ID3D12Device*>(gpudevice->nativeDevice());

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (d3d12Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK) {
    }

    ImGui_ImplDX12_Init(d3d12Device, graphics::D3D12Backend::CHAIN_NUM_FRAMES,
        graphics::D3D12Backend::Format[(uint32_t) colorTargetFormat ],
        g_pd3dSrvDescHeap,
        g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());
}

void Imgui::newFrame() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Imgui::draw(const graphics::BatchPointer& batch) {
    auto d3d12CommandList = static_cast<graphics::D3D12BatchBackend*>(batch.get())->_commandList.Get();
    d3d12CommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), d3d12CommandList);
}

bool Imgui::customEventCallback(HWND win, UINT msg, WPARAM wparam, LPARAM lparam) {
    return ImGui_ImplWin32_WndProcHandler(win, msg, wparam, lparam);
}

void Imgui::createDeviceObjects() {
    ImGui_ImplDX12_CreateDeviceObjects();
}

void Imgui::invalidateDeviceObjects() {
    ImGui_ImplDX12_InvalidateDeviceObjects();
}

void Imgui::standardPostSceneRenderCallback(
    graphics::RenderArgs& args) {

    ImGui::Render();
    draw(args.batch);
}


#endif //win32
