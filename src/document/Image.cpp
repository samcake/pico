// Image.cpp 
//
// Sam Gateau - February 2021
//
// Wrapper class over stb_image at http://nothings.org/stb
// 
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
#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace document;


bool Image::loadFromMemory(ImageMimeType type, const std::vector<uint8_t>& bytes) {
    // preserve_channels true: Use channels stored in the image file.
     // false: force 32-bit textures for common Vulkan compatibility. It appears
     // that some GPU drivers do not support 24-bit images for Vulkan
    int req_comp = 4;

    int w, h, comp;
    auto stbi_pixels = stbi_load_from_memory(bytes.data(), bytes.size(), &w, &h, &comp, req_comp);
    if (stbi_pixels) {
        _desc.width = w;
        _desc.height = h;
        _desc.components = comp;
        _desc.mimeType = type;

        _desc.pixels_size = w * h * req_comp;

        _pixels.resize(_desc.pixels_size, 0);
        memcpy(_pixels.data(), stbi_pixels, _desc.pixels_size);
        
        return true;
    }
    return false;
}

