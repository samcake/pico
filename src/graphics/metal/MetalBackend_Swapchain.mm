// MetalBackend_Swapchain.mm
// CAMetalLayer swapchain + framebuffer for pico
//
// Sam Gateau / pico - 2024
// MIT License
#ifdef __APPLE__

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "MetalBackend.h"
#include <iostream>

using namespace graphics;

// ---------------------------------------------------------------------------
// MetalSwapchainBackend helpers
// ---------------------------------------------------------------------------
void MetalSwapchainBackend::acquireNextDrawable() {
    _currentDrawable = [_layer nextDrawable];
}

void MetalSwapchainBackend::releaseDrawable() {
    _currentDrawable = nil;
}

// ---------------------------------------------------------------------------
// createSwapchain
// ---------------------------------------------------------------------------
SwapchainPointer MetalBackend::createSwapchain(const SwapchainInit& init) {
    auto* sw = new MetalSwapchainBackend();
    sw->_init = init;

    NSView* view = (__bridge NSView*)init.windowHandle;

    // Attach (or reuse) a CAMetalLayer on the NSView
    CAMetalLayer* layer = nil;
    if ([view.layer isKindOfClass:[CAMetalLayer class]]) {
        layer = (CAMetalLayer*)view.layer;
    } else {
        layer = [CAMetalLayer layer];
        view.layer = layer;
        view.wantsLayer = YES;
    }

    layer.device = _device;

    // CAMetalLayer only supports BGRA formats — force the conversion
    MTLPixelFormat layerFmt = MTLPixelFormatBGRA8Unorm;
    MTLPixelFormat requestedFmt = MetalBackend::Format[(uint32_t)init.colorBufferFormat];
    if (requestedFmt == MTLPixelFormatRGBA8Unorm_sRGB)
        layerFmt = MTLPixelFormatBGRA8Unorm_sRGB;
    layer.pixelFormat = layerFmt;
    layer.framebufferOnly = YES;

    // Use the view's actual backing (pixel) dimensions for the drawable
    // This ensures the swapchain matches the view regardless of what init.width/height say
    CGFloat scale = view.window ? view.window.backingScaleFactor : [NSScreen mainScreen].backingScaleFactor;
    layer.contentsScale = scale;
    NSRect backing = [view convertRectToBacking:view.bounds];
    uint32_t pixelW = (uint32_t)backing.size.width;
    uint32_t pixelH = (uint32_t)backing.size.height;
    layer.drawableSize = CGSizeMake((CGFloat)pixelW, (CGFloat)pixelH);

    sw->_layer = layer;
    sw->_init.width  = pixelW;
    sw->_init.height = pixelH;

    // Depth texture at pixel resolution
    if (init.depthBuffer) {
        MTLTextureDescriptor* dd = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                         width:pixelW
                                        height:pixelH
                                     mipmapped:NO];
        dd.usage        = MTLTextureUsageRenderTarget;
        dd.storageMode  = MTLStorageModePrivate;
        sw->_depthTexture = [_device newTextureWithDescriptor:dd];
    }

    return SwapchainPointer(sw);
}

void MetalBackend::resizeSwapchain(const SwapchainPointer& swapchain,
                                    uint32_t width, uint32_t height) {
    auto* sw = static_cast<MetalSwapchainBackend*>(swapchain.get());
    sw->_layer.drawableSize = CGSizeMake((CGFloat)width, (CGFloat)height);
    sw->_init.width  = width;
    sw->_init.height = height;

    if (sw->_depthTexture && width > 0 && height > 0) {
        MTLTextureDescriptor* dd = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                         width:width
                                        height:height
                                     mipmapped:NO];
        dd.usage       = MTLTextureUsageRenderTarget;
        dd.storageMode = MTLStorageModePrivate;
        sw->_depthTexture = [_device newTextureWithDescriptor:dd];
    }
}

// ---------------------------------------------------------------------------
// pixelFormatToMTL helper
// ---------------------------------------------------------------------------
MTLPixelFormat MetalBackend::pixelFormatToMTL(PixelFormat fmt) {
    return Format[(uint32_t)fmt];
}

#endif // __APPLE__
