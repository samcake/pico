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
#include "VKBackend.h"
#include <fstream>
#include <sstream>

#include <vector>

using namespace graphics;

#ifdef PICO_VULKAN


//#define USE_DXC
//
//#define ThrowIfFailed(result) if (FAILED((result))) picoLog("D3D12Backend_Shader FAILED !!!");
//
//
const std::string VKShaderBackend::ShaderTypes[] = {
    "nope",
#ifdef USE_DXC
    "vs_6_1",
    "ps_6_1",
    "cs_6_1",
#else
    "vs_5_1",
    "ps_5_1",
    "cs_5_1",
#endif
    "lib_6_3",
};

VKShaderBackend::VKShaderBackend() {

}

VKShaderBackend::~VKShaderBackend() {

}

#ifdef USE_DXC
class PicoDXCIncludeHandler : public IDxcIncludeHandler {
public:
    std::atomic<ULONG> m_cRef = { 0 };

    ULONG AddRef()
    {
        return m_cRef++;
    }
    ULONG Release()
    {
        // Decrement the object's internal counter.
        ULONG ulRefCount = m_cRef--;
        if (0 == ulRefCount)
        {
            delete this;
        }
        return ulRefCount;
    }

    virtual ~PicoDXCIncludeHandler() {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) {
        // Always set out parameter to NULL, validating it first.
        if (!ppvObj)
            return E_INVALIDARG;
        *ppvObj = NULL;
        if (riid == IID_IUnknown /* || riid == IID_IMAPIProp ||
            riid == IID_IMAPIStatus*/)
        {
            // Increment the reference count and return the pointer.
            *ppvObj = (LPVOID)this;
            AddRef();
            return NOERROR;
        }
        return E_NOINTERFACE;
    }
    ComPtr<IDxcUtils> _utils;
    const Shader* _src = nullptr;

    PicoDXCIncludeHandler(ComPtr<IDxcUtils> pUtils, const Shader* src) : _utils(pUtils), _src(src) {}

    HRESULT STDMETHODCALLTYPE LoadSource(
        _In_ LPCWSTR pFilename,                                 // Candidate filename.
        _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource  // Resultant source object for included file, nullptr if not found.
    ) {
        auto includedFilename = core::to_string(std::wstring(pFilename));

        includedFilename = includedFilename.substr(2);
        auto shaderFilename = _src->getShaderDesc().watcher_file;
     
        if (_src && _src->getShaderDesc().includes.size()) {
            auto& includes = _src->getShaderDesc().includes;
            auto i = includes.find(includedFilename);
            if (i != includes.end()) {
                ComPtr<IDxcBlobEncoding> pSource;
                _utils->CreateBlob(i->second().data(), i->second().size(), CP_UTF8, pSource.GetAddressOf());
                *ppIncludeSource = pSource.Detach();
            } else {
                picoLog("Missing include candidate for <" + includedFilename + "> from shader " + shaderFilename + " ... ");// +parentData);
            }
        } else {
            picoLog("Failed to include <" + includedFilename + "> no includes provided from shader " + shaderFilename + " ... ");// +parentData);
        }
        return S_OK;
    }
};

bool PicoDXCCompileShader(Shader* shader, const std::string& source) {
    ComPtr<IDxcUtils> pUtils;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(pUtils.GetAddressOf()));
    auto* includer = new PicoDXCIncludeHandler(pUtils, shader);

    // ComPtr<IDxcIncludeHandler> includeHandler(includer);
    // pUtils->CreateDefaultIncludeHandler(includeHandler.ReleaseAndGetAddressOf());

    ComPtr<IDxcCompiler3> compiler;
    HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

    ComPtr<IDxcBlobEncoding> pSource;
    //  auto wsource = core::to_wstring(source);
    auto& wsource = source;
    pUtils->CreateBlob(wsource.c_str(), wsource.size(), CP_UTF8, pSource.GetAddressOf());

    std::vector<LPWSTR> arguments;
    //-E for the entry point (eg. PSMain)
    arguments.push_back(L"-E");
    auto wentry = core::to_wstring(shader->getShaderDesc().entryPoint);

    arguments.push_back((LPWSTR)wentry.c_str());

    //-T for the target profile (eg. ps_6_2)
    arguments.push_back(L"-T");

    std::string target(D3D12ShaderBackend::ShaderTypes[(int)shader->getShaderDesc().type]);

    auto wtarget = core::to_wstring(target);
    arguments.push_back((LPWSTR)wtarget.c_str());

    //Strip reflection data and pdbs (see later)
    arguments.push_back(L"-Qstrip_debug");
    arguments.push_back(L"-Qstrip_reflect");

    arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
    arguments.push_back(DXC_ARG_DEBUG); //-Zi
    arguments.push_back(DXC_ARG_PACK_MATRIX_ROW_MAJOR); //-Zp
    /*
        for (const std::wstring& define : defines)
        {
            arguments.push_back(L"-D");
            arguments.push_back(define.c_str());
        }*/

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = pSource->GetBufferPointer();
    sourceBuffer.Size = pSource->GetBufferSize();
    sourceBuffer.Encoding = 0;

    ComPtr<IDxcResult> pCompileResult;
    //hr = compiler->Compile(&sourceBuffer, arguments.data(), (uint32_t)arguments.size(), nullptr, IID_PPV_ARGS(pCompileResult.GetAddressOf())));
    hr = compiler->Compile(&sourceBuffer, (LPCWSTR*)arguments.data(), (uint32_t)arguments.size(), includer, IID_PPV_ARGS(pCompileResult.GetAddressOf()));

    //Error Handling
    ComPtr<IDxcBlobUtf8> pErrors;
    pCompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.GetAddressOf()), nullptr);
    if (pErrors && pErrors->GetStringLength() > 0)
    {
        //      MyLogFunction(Error, (char*)pErrors->GetBufferPointer());

        std::string file = (shader->getShaderDesc().watcher_file.empty() ?
            (shader->getShaderDesc().url.empty() ?
                "no source file" :
                shader->getShaderDesc().url) :
            shader->getShaderDesc().watcher_file);
        picoLog((target + " " + shader->getShaderDesc().entryPoint + "\n"
            + "    " + file + "\n"
            + (char*)pErrors->GetBufferPointer()));

        //errorBlob->Release();
    }
    ComPtr<IDxcBlob> shaderBlob;
    pCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);


    std::string file = (shader->getShaderDesc().watcher_file.empty() ?
        (shader->getShaderDesc().url.empty() ?
            "no source file" :
            shader->getShaderDesc().url) :
        shader->getShaderDesc().watcher_file);

    dynamic_cast<D3D12ShaderBackend*>(shader)->_shaderBlob = shaderBlob;
    return true;
}

#else

//class PicoID3DInclude : public ID3DInclude {
//
//    const Shader* _src = nullptr;
//
//
//
//    HRESULT Open(D3D_INCLUDE_TYPE IncludeType,
//        LPCSTR           pFileName,
//        LPCVOID          pParentData,
//        LPCVOID* ppData,
//        UINT* pBytes) override {
//
//        auto includedFilename = std::string(pFileName);
//        auto shaderFilename = _src->getShaderDesc().watcher_file;
//        auto parentData = (pParentData != 0 ? std::string((char*)pParentData) : std::string("UNKNOWN"));
//
//        if (_src && _src->getShaderDesc().includes.size()) {
//            auto& includes = _src->getShaderDesc().includes;
//            auto i = includes.find(includedFilename);
//            if (i != includes.end()) {
//                (*ppData) = i->second().data();
//                (*pBytes) = i->second().size();
//            } else {
//
//                picoLog("Missing include candidate for <" + includedFilename + "> from shader " + shaderFilename + " ... " + parentData);
//            }
//        } else {
//            picoLog("Failed to include <" + includedFilename + "> no includes provided from shader " + shaderFilename + " ... " + parentData);
//        }
//        return S_OK;
//    }
//
//    HRESULT Close(LPCVOID pData) override {
//
//        return S_OK;
//    }
//
//public:
//    PicoID3DInclude(const Shader* src) : _src(src) {
//
//    }
//};
//
//
//bool PicoFXCCompileShader(Shader* shader, const std::string& source) {
//    std::string target(D3D12ShaderBackend::ShaderTypes[(int)shader->getShaderDesc().type]);
//
//    ID3DBlob* shaderBlob;
//    ID3DBlob* errorBlob;
//
//    PicoID3DInclude* includer = new PicoID3DInclude(shader);
//
//    HRESULT hr = D3DCompile(
//        source.c_str(),
//        source.size(),
//        shader->getShaderDesc().url.c_str(),
//        nullptr,
//        includer,
//        shader->getShaderDesc().entryPoint.c_str(),
//        target.c_str(),
//        0, 0,
//        &shaderBlob,
//        &errorBlob);
//
//    if (FAILED(hr)) {
//        if (errorBlob) {
//            std::string file = (shader->getShaderDesc().watcher_file.empty() ?
//                (shader->getShaderDesc().url.empty() ?
//                    "no source file" :
//                    shader->getShaderDesc().url) :
//                shader->getShaderDesc().watcher_file);
//            picoLog((target + " " + shader->getShaderDesc().entryPoint + "\n"
//                + "    " + file + "\n"
//                + (char*)errorBlob->GetBufferPointer()));
//            errorBlob->Release();
//        }
//
//        if (shaderBlob)
//            shaderBlob->Release();
//
//
//        return false;
//    }
//
//    dynamic_cast<D3D12ShaderBackend*>(shader)->_shaderBlob = shaderBlob;
//    return true;
//}



#endif

bool VKBackend::compileShader(Shader* shader, const std::string& source) {
    if (!shader) {
        return false;
    }

#ifdef USE_DXC
    return PicoDXCCompileShader(shader, source);
#else
 //   return PicoFXCCompileShader(shader, source);
#endif   
    return false;
}



bool VKBackend::compileShaderLib(Shader* shader, const std::string& source) {
    if (!shader) {
        return false;
    }

//    ComPtr<IDxcUtils> pUtils;
//    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(pUtils.GetAddressOf()));
//
//    ComPtr<IDxcIncludeHandler> includeHandler;
//    pUtils->CreateDefaultIncludeHandler(includeHandler.ReleaseAndGetAddressOf());
//
//    ComPtr<IDxcCompiler3> compiler;
//    HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
//
//    ComPtr<IDxcBlobEncoding> pSource;
//  //  auto wsource = core::to_wstring(source);
//    auto& wsource = source;
//    pUtils->CreateBlob(wsource.c_str(), wsource.size(), CP_UTF8, pSource.GetAddressOf());
//
//    std::vector<LPWSTR> arguments;
//    //-E for the entry point (eg. PSMain)
//    arguments.push_back(L"-E");
//    auto wentry = core::to_wstring(shader->getShaderDesc().entryPoint);
//
//    arguments.push_back((LPWSTR) wentry.c_str());
//
//    //-T for the target profile (eg. ps_6_2)
//    arguments.push_back(L"-T");
//
//    std::string target(D3D12ShaderBackend::ShaderTypes[(int)shader->getShaderDesc().type]);
//
//    auto wtarget = core::to_wstring(target);
//    arguments.push_back((LPWSTR) wtarget.c_str());
//
//    //Strip reflection data and pdbs (see later)
//    arguments.push_back(L"-Qstrip_debug");
//    arguments.push_back(L"-Qstrip_reflect");
//
//    arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
//    arguments.push_back(DXC_ARG_DEBUG); //-Zi
//    arguments.push_back(DXC_ARG_PACK_MATRIX_ROW_MAJOR); //-Zp
///*
//    for (const std::wstring& define : defines)
//    {
//        arguments.push_back(L"-D");
//        arguments.push_back(define.c_str());
//    }*/
//
//    DxcBuffer sourceBuffer;
//    sourceBuffer.Ptr = pSource->GetBufferPointer();
//    sourceBuffer.Size = pSource->GetBufferSize();
//    sourceBuffer.Encoding = 0;
//
//    ComPtr<IDxcResult> pCompileResult;
//    //hr = compiler->Compile(&sourceBuffer, arguments.data(), (uint32_t)arguments.size(), nullptr, IID_PPV_ARGS(pCompileResult.GetAddressOf())));
//    hr = compiler->Compile(&sourceBuffer, (LPCWSTR*)arguments.data(), (uint32_t)arguments.size(), nullptr, IID_PPV_ARGS(pCompileResult.GetAddressOf()));
//
//    //Error Handling
//    ComPtr<IDxcBlobUtf8> pErrors;
//    pCompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.GetAddressOf()), nullptr);
//    if (pErrors && pErrors->GetStringLength() > 0)
//    {
//  //      MyLogFunction(Error, (char*)pErrors->GetBufferPointer());
//
//        std::string file = (shader->getShaderDesc().watcher_file.empty() ?
//            (shader->getShaderDesc().url.empty() ?
//                "no source file" :
//                shader->getShaderDesc().url) :
//            shader->getShaderDesc().watcher_file);
//        picoLog((target + " " + shader->getShaderDesc().entryPoint + "\n"
//            + "    " + file + "\n"
//            + (char*)pErrors->GetBufferPointer()));
//
//        //errorBlob->Release();
//    }
//    ComPtr<IDxcBlob> shaderBlob;
//    pCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
//
//
//    std::string file = (shader->getShaderDesc().watcher_file.empty() ?
//        (shader->getShaderDesc().url.empty() ?
//            "no source file" :
//            shader->getShaderDesc().url) :
//        shader->getShaderDesc().watcher_file);
//    picoLog((target + " " + shader->getShaderDesc().entryPoint + "\n"
//        + "    \n"
//        + (char*)shaderBlob->GetBufferPointer()));

  /*  IDxcCompiler* compiler;
    pUtils->


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
        &errorBlob);*/
   /*
   if (FAILED(hr)) {
        if (errorBlob) {
            std::string file = (shader->getShaderDesc().watcher_file.empty() ?
                (shader->getShaderDesc().url.empty() ?
                    "no source file" :
                    shader->getShaderDesc().url) :
                shader->getShaderDesc().watcher_file);
            picoLog((target + " " + shader->getShaderDesc().entryPoint + "/n"
                + "    " + file + "/n"
                + (char*)errorBlob->GetBufferPointer()));
            errorBlob->Release();
        }

        if (shaderBlob)
            shaderBlob->Release();


        return false;
    }*/
 //   dynamic_cast<D3D12ShaderBackend*>(shader)->_shaderLibBlob = shaderBlob;
    return true;
}

ShaderPointer VKBackend::createShader(const ShaderInit& init) {

    std::string target(VKShaderBackend::ShaderTypes[(int) init.type]);

    std::string source;
    if (!init.url.empty()) {
        std::ifstream file(init.url, std::ifstream::in);
        source = (std::stringstream() << file.rdbuf()).str();
    } else if (init.sourceGetter) {
        source = init.sourceGetter();
    }

    // Allocate the shader
    auto shader = std::make_shared< VKShaderBackend>();
    shader->_shaderDesc = init;

    // and now compile
    if (init.type == ShaderType::RAYTRACING) {
        shader->_programDesc.type = PipelineType::RAYTRACING;
        if (VKBackend::compileShader(shader.get(), source)) {
            if (shader->hasWatcher()) {
                auto shaderCompiler = std::function([&](Shader* sha, const std::string& src) -> bool {
                    return compileShader(sha, src);
                    });

                Shader::registerToWatcher(shader, shaderCompiler);
            }
            return shader;
        }
    } else {
        if (VKBackend::compileShader(shader.get(), source)) {
            if (shader->hasWatcher()) {
                auto shaderCompiler = std::function([&](Shader* sha, const std::string& src) -> bool {
                    return compileShader(sha, src);
                    });

                Shader::registerToWatcher(shader, shaderCompiler);
            }
            return shader;
        }
    }

    return ShaderPointer();
}


ShaderPointer VKBackend::createProgram(const ProgramInit& init) {
    VKShaderBackend* program = nullptr;
    // check the shaders are valid
    if (init.type == PipelineType::GRAPHICS) {
        if (init.shaderLib.at("VERTEX") && init.shaderLib.at("PIXEL")) {

            program = new VKShaderBackend();
            program->_programDesc = init;

            program->_shaderDesc.type = ShaderType::PROGRAM;
        }
    }
    else if (init.type == PipelineType::RAYTRACING) {
        program = new VKShaderBackend();
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
