// D3D12Backend_Shader.cpp
//
// Sam Gateau - 2020/1/1
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

using namespace poco;

#ifdef WIN32

D3D12PipelineStateBackend::D3D12PipelineStateBackend() {

}

D3D12PipelineStateBackend::~D3D12PipelineStateBackend() {

}

PipelineStatePointer D3D12Backend::createPipelineState(const PipelineStateInit& init) {
    auto pso = new D3D12PipelineStateBackend();

    return PipelineStatePointer(pso);
}

D3D12ShaderBackend::D3D12ShaderBackend() {

}

D3D12ShaderBackend::~D3D12ShaderBackend() {

}

ShaderPointer D3D12Backend::createShader(const ShaderInit& init) {
    auto shader = new D3D12ShaderBackend();

    return ShaderPointer(shader);
}

#endif
