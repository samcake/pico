// Image.h 
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
#pragma once

#include <vector>
#include <document/dllmain.h>

namespace document {

    enum class ImageMimeType {
        JPEG = 0,
        PNG,
    };

    struct ImageDesc {
        uint32_t width;
        uint32_t height;
        uint8_t components;
        ImageMimeType mimeType;
        uint64_t pixels_size;
    };

    class Image {
    public:
        ImageDesc _desc;
        std::vector<uint8_t> _pixels;

        bool loadFromMemory(ImageMimeType type, const std::vector<uint8_t>& bytes);
    };
}
