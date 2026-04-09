// MetalBackend_Framebuffer.mm
// Custom render-target framebuffer for pico Metal backend
//
// Sam Gateau / pico - 2024
// MIT License
#ifdef __APPLE__

#include "MetalBackend.h"

using namespace graphics;

FramebufferPointer MetalBackend::createFramebuffer(const FramebufferInit& init) {
    auto* fb  = new MetalFramebufferBackend();
    fb->initFromDesc(init);   // sets _init from within the subclass

    for (auto& ct : init.colorTargets) {
        if (ct) {
            auto* tex = static_cast<MetalTextureBackend*>(ct.get());
            fb->_colorTextures.push_back(tex->_texture);
        } else {
            fb->_colorTextures.push_back(nil);
        }
    }

    if (init.depthStencilTarget) {
        auto* tex = static_cast<MetalTextureBackend*>(init.depthStencilTarget.get());
        fb->_depthTexture = tex->_texture;
    }

    return FramebufferPointer(fb);
}

#endif // __APPLE__
