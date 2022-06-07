// Resource.cpp
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
#include "Resource.h"

using namespace graphics;

Resource::Resource() {

}

Resource::~Resource() {

}

Buffer::Buffer() :
	_init{},
	_bufferSize(0),
	_cpuMappedAddress(nullptr)
{

}

Buffer::~Buffer() {

}

Texture::Texture() :
	_init{}
{

}

Texture::~Texture() {

}

Geometry::Geometry() :
	_init{}
{

}

Geometry::~Geometry() {

}

std::pair<UploadSubresourceLayoutArray, uint64_t> Texture::evalUploadSubresourceLayout(const TexturePointer& dest, const std::vector<uint32_t>& subresources) {
    // find amount of data required to fit all the init data
    uint64_t bufferSize = 0;
    UploadSubresourceLayoutArray updloadLayout;
    // if no subresoruces, assume all
    if (subresources.empty()) {
        uint32_t s = 0;
        for (const auto& id : dest->_init.initData) {
            if (id.size()) {
                UploadSubresourceLayout ul;
                ul.subresource = s;
                ul.byteOffset = bufferSize;
                ul.byteLength = dest->_init.initData[s].size();
                bufferSize += ul.byteLength;
                updloadLayout.emplace_back(ul);
            }
            s++;
        }
    } else {
        for (const auto& s : subresources) {
            if (dest->_init.initData.size() > s) {
                if (dest->_init.initData[s].size()) {
                    UploadSubresourceLayout ul;
                    ul.subresource = s;
                    ul.byteOffset = bufferSize;
                    ul.byteLength = dest->_init.initData[s].size();
                    bufferSize += ul.byteLength;
                    updloadLayout.emplace_back(ul);
                }
            }
        }
    }

    return { updloadLayout, bufferSize };
}

