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
#define ThrowIfFailed(result) if (FAILED((result))) picoLog("D3D12Backend_Shader FAILED !!!");


const std::string D3D12ShaderBackend::ShaderTypes[] = {
    "nope",
    "vs_5_1",
    "ps_5_1",
    "cs_5_1"
};

D3D12ShaderBackend::D3D12ShaderBackend() {

}

D3D12ShaderBackend::~D3D12ShaderBackend() {

}

class PicoID3DInclude : public ID3DInclude {

    const Shader* _src = nullptr;
    


    HRESULT Open(   D3D_INCLUDE_TYPE IncludeType,
                    LPCSTR           pFileName,
                    LPCVOID          pParentData,
                    LPCVOID* ppData,
                    UINT* pBytes) override {

        auto includedFilename = std::string(pFileName);
        auto shaderFilename = _src->getShaderDesc().url;
        auto parentData = (pParentData != 0 ? std::string((char*)pParentData) : std::string("UNKNOWN"));

        if (_src && _src->getShaderDesc().includes.size()) {
            auto& includes = _src->getShaderDesc().includes;
            auto i = includes.find(includedFilename);
            if (i != includes.end()) {
                (*ppData) = i->second().data();
                (*pBytes) = i->second().size();
            } else {

                picoLog("Missing include candidate for <" + includedFilename + "> from shader " + shaderFilename  + " ... " + parentData);
            }
        } else {
            picoLog("Failed to include <" + includedFilename + "> no includes provided from shader " + shaderFilename + " ... " + parentData);
        }
        return S_OK;
    }

    HRESULT Close(LPCVOID pData) override {

        return S_OK;
    }

public:
    PicoID3DInclude(const Shader* src) : _src( src ) {

    }
};

bool D3D12Backend::compileShader(Shader* shader, const std::string& source) {
    if (!shader) {
        return false;
    }

    std::string target(D3D12ShaderBackend::ShaderTypes[(int)shader->getShaderDesc().type]);

    ID3DBlob* shaderBlob;
    ID3DBlob* errorBlob;

    PicoID3DInclude* includer = new PicoID3DInclude(shader);

    HRESULT hr = D3DCompile(
        source.c_str(),
        source.size(),
        shader->getShaderDesc().url.c_str(),
        nullptr,
        includer,
        shader->getShaderDesc().entryPoint.c_str(),
        target.c_str(),
        0, 0,
        &shaderBlob,
        &errorBlob);

    if (FAILED(hr)) {
        if (errorBlob) {
            std::string file = (shader->getShaderDesc().watcher_file.empty() ?
                                    (shader->getShaderDesc().url.empty() ?
                                        "no source file" :
                                        shader->getShaderDesc().url) :
                                    shader->getShaderDesc().watcher_file);
            picoLog(  (target + " " + shader->getShaderDesc().entryPoint + "/n"
                      + "    " + file + "/n"
                      + (char*)errorBlob->GetBufferPointer()));
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

    std::string source;
    if (!init.url.empty()) {
        std::ifstream file(init.url, std::ifstream::in);
        source = (std::stringstream() << file.rdbuf()).str();
    } else if (init.sourceGetter) {
        source = init.sourceGetter();
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
