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

bool D3D12Backend::compileShader(Shader* shader, const std::string& source) {
    if (!shader) {
        return false;
    }

    std::string target(D3D12ShaderBackend::ShaderTypes[(int)shader->getShaderDesc().type]);

    ID3DBlob* shaderBlob;
    ID3DBlob* errorBlob;

    HRESULT hr = D3DCompile(
        source.c_str(),
        source.size(),
        shader->getShaderDesc().url.c_str(),
        nullptr,
        nullptr,
        shader->getShaderDesc().entryPoint.c_str(),
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


        return false;
    }
    dynamic_cast<D3D12ShaderBackend*>(shader)->_shaderBlob = shaderBlob;
    return true;
}

ShaderPointer D3D12Backend::createShader(const ShaderInit& init) {

    std::string target(D3D12ShaderBackend::ShaderTypes[(int) init.type]);
    std::string source = init.source;

    if (!init.url.empty()) {
        std::ifstream file(init.url, std::ifstream::in);
        source = (std::stringstream() << file.rdbuf()).str();
    }

    // Allocate the shader
    auto shader = std::make_shared< D3D12ShaderBackend>();
    shader->_shaderDesc = init;

    // and now compile
    if ( D3D12Backend::compileShader(shader.get(), source) ) {
        if (shader->hasWatcher()) {
            auto shaderCompiler = std::function( [&] (Shader* sha, const std::string& src) -> bool {
                return compileShader(sha, src);
            });

            Shader::registerToWatcher(shader, shaderCompiler);
        }
        return shader;
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

    auto program_ptr = ShaderPointer(program);
    if (program_ptr->hasWatcher()) {
        auto programLinker = std::function([&](Shader* sha) -> bool {
            return true;
        });

        Shader::registerToWatcher(program_ptr, nullptr, programLinker);
    }

    return program_ptr;
}

#endif