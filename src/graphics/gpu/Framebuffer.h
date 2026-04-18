// Framebuffer.h 
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
#pragma once

#include "gpu.h"
#include <vector>

namespace graphics {

    const uint32_t FRAMEBUFFER_MAX_NUM_COLOR_TARGETS = 8;

    using TextureArray = std::vector<TexturePointer>;

    // Draw into externally-provided textures. Single slot, no lifecycle ownership.
    struct FramebufferInit {
        uint32_t width = 0; // default is 0 meaning the width and height will be evaluated from the render target size
        uint32_t height = 0;

        TextureArray colorTargets;

        TexturePointer depthStencilTarget;
    };

    // Autonomous N-buffered framebuffer. Device allocates and owns all textures.
    struct FramebufferInit_Swapable {
        uint32_t    width{ 0 };
        uint32_t    height{ 0 };
        uint32_t    numBuffers{ 2 };
        bool        depthBuffer{ false };
        PixelFormat colorFormat{ PixelFormat::R8G8B8A8_UNORM_SRGB };
        PixelFormat depthFormat{ PixelFormat::D32_FLOAT };
    };

    class Framebuffer {
    protected:
        // Framebuffer is created from the device
        friend class Device;
        Framebuffer();

        FramebufferInit _init;

        // Chain support: populated by both init paths for resize and texture access
        std::vector<TexturePointer> _colorBuffers; // one entry per buffer slot
        std::vector<TexturePointer> _depthBuffers; // one entry per buffer slot (empty = no depth)

        uint32_t _numBuffers{ 1 };

    public:
        ~Framebuffer();

        enum BufferMask {
            BUFFER_COLOR0 = 1,
            BUFFER_COLOR1 = 2,
            BUFFER_COLOR2 = 4,
            BUFFER_COLOR3 = 8,
            BUFFER_COLOR4 = 16,
            BUFFER_COLOR5 = 32,
            BUFFER_COLOR6 = 64,
            BUFFER_COLOR7 = 128,
            BUFFER_COLORS = 0x000000FF,

            BUFFER_DEPTH = 0x40000000,
            BUFFER_STENCIL = 0x80000000,
            BUFFER_DEPTHSTENCIL = 0xC0000000,
        };

        uint32_t width() const { return _init.width; }
        uint32_t height() const { return _init.height; }
        uint32_t numBuffers() const { return _numBuffers; }

        uint8_t _currentIndex{ 0 };
        uint8_t currentIndex() const { return _currentIndex; }
        void advanceIndex() { _currentIndex = (_currentIndex + 1) % (uint8_t)_numBuffers; }

        TexturePointer colorTexture(uint32_t bufferIndex = 0) const {
            return (bufferIndex < _colorBuffers.size()) ? _colorBuffers[bufferIndex] : nullptr;
        }
        TexturePointer depthTexture(uint32_t bufferIndex = 0) const {
            return (bufferIndex < _depthBuffers.size()) ? _depthBuffers[bufferIndex] : nullptr;
        }
    };
}