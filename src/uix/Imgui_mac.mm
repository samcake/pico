// Imgui_mac.mm
// macOS + Metal ImGui backend for pico
//
// Sam Gateau / pico - 2024
// MIT License
#ifdef __APPLE__

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "Imgui.h"
#include "Window.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_metal.h"
#include "imgui/imgui_impl_osx.h"

#include <graphics/gpu/Device.h>
#include <graphics/gpu/Batch.h>
#include <graphics/render/Renderer.h>

// Forward-declared in MetalBackend.h — we use void* nativeDevice()
// and cast to id<MTLDevice> here.

// Access to the Metal batch's current render encoder
// declared in MetalBackend.h but we can't include that Obj-C++ header here easily.
// We use a free function instead:
namespace graphics {
    id<MTLRenderCommandEncoder> MetalBatch_getCurrentEncoder(const BatchPointer& batch);
    id<MTLCommandBuffer>        MetalBatch_getCurrentCommandBuffer(const BatchPointer& batch);
    MTLRenderPassDescriptor*    MetalBatch_getCurrentRenderPassDescriptor(const BatchPointer& batch);
}

using namespace uix;

// Cached view so newFrame() can set DisplaySize
static NSView* g_picoView = nil;

void Imgui::create() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
}

void Imgui::destroy() {
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplOSX_Shutdown();
    ImGui::DestroyContext();
    g_picoView = nil;
}

void Imgui::setup(const WindowPointer& win, const graphics::DevicePointer& gpudevice, graphics::PixelFormat colorTargetFormat) {
    id<MTLDevice> device = (__bridge id<MTLDevice>)gpudevice->nativeDevice();

    // win->nativeWindow() returns NSView* (PicoView)
    g_picoView = (__bridge NSView*)win->nativeWindow();

    ImGui_ImplMetal_Init(device);
    ImGui_ImplOSX_Init();
}

void Imgui::newFrame() {
    ImGui_ImplOSX_NewFrame(g_picoView);
    ImGui::NewFrame();
}

void Imgui::draw(const graphics::BatchPointer& batch) {
    auto encoder = graphics::MetalBatch_getCurrentEncoder(batch);
    if (!encoder) return;
    // Call Metal new frame here with the actual render pass descriptor so the
    // pipeline is created with the correct format/sampleCount
    auto* rpd = graphics::MetalBatch_getCurrentRenderPassDescriptor(batch);
    ImGui_ImplMetal_NewFrame(rpd);
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), graphics::MetalBatch_getCurrentCommandBuffer(batch), encoder);
}

bool Imgui::customEventCallback(void* nsEvent) {
    if (!ImGui::GetCurrentContext() || !g_picoView) return false;
    NSEvent* event = (__bridge NSEvent*)nsEvent;
    return ImGui_ImplOSX_HandleEvent(event, g_picoView);
}

void Imgui::createDeviceObjects() {
    ImGui_ImplMetal_CreateDeviceObjects(nil);
}

void Imgui::invalidateDeviceObjects() {
    ImGui_ImplMetal_DestroyDeviceObjects();
}

void Imgui::standardPostSceneRenderCallback(graphics::RenderArgs& args) {
    ImGui::Render();
    draw(args.batch);
}

#endif // __APPLE__
