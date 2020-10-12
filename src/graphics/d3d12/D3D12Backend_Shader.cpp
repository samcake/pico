// D3D12Backend_Shader.cpp
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
#include <fstream>
#include <sstream>

#include <vector>

using namespace graphics;

#ifdef _WINDOWS
#define ThrowIfFailed(result) if (FAILED((result))) picoLog() << "FAILED !!!/n";


const std::string D3D12ShaderBackend::ShaderTypes[] = {
    "nope",
    "vs_5_1",
    "ps_5_1",
};

D3D12ShaderBackend::D3D12ShaderBackend() {

}

D3D12ShaderBackend::~D3D12ShaderBackend() {

}

ShaderPointer D3D12Backend::createShader(const ShaderInit& init) {

    
    std::string target(D3D12ShaderBackend::ShaderTypes[(int) init.type]);
    std::string source = init.source;

    if (!init.url.empty()) {
        std::ifstream file(init.url, std::ifstream::in);
        source = (std::stringstream() << file.rdbuf()).str();
    }

    
    if (!source.empty()) {
        ID3DBlob* shaderBlob;
        ID3DBlob* errorBlob;
        

        HRESULT hr = D3DCompile(
            source.c_str(),
            source.size(),
            init.url.c_str(),
            nullptr,
            nullptr,
            init.entryPoint.c_str(),
            target.c_str(),
            0, 0,
            &shaderBlob,
            &errorBlob);

        if (FAILED(hr)) {
            if (errorBlob) {
                picoLog() << (char*)errorBlob->GetBufferPointer();
             //   OutputDebugStringA((char*)errorBlob->GetBufferPointer());
                errorBlob->Release();
            }

            if (shaderBlob)
                shaderBlob->Release();


            return ShaderPointer();
        }

        auto shader = new D3D12ShaderBackend();

        shader->_shaderDesc = init;
        shader->_shaderBlob = shaderBlob;

//        *blob = shaderBlob;
 //       shader->_shaderBlob = bytecodeBlob;

        return ShaderPointer(shader);
    }

    return ShaderPointer();
}


ShaderPointer D3D12Backend::createProgram(const ProgramInit& init) {
    D3D12ShaderBackend* program = nullptr;
    // check the shaders are valid
    if (init.pixelShader && init.vertexShader) {
        program = new D3D12ShaderBackend();
        program->_programDesc = init;

        program->_shaderDesc.type = ShaderType::PROGRAM;
    }

    return ShaderPointer(program);
}

#endif
