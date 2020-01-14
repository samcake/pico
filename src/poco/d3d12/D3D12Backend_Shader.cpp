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
#include <fstream>
#include <sstream>
#include "Api.h"

using namespace poco;

#ifdef WIN32
#define ThrowIfFailed(result) if (FAILED((result))) pocoLog() << "FAILED !!!/n";


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
    std::string source;

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
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
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

D3D12PipelineStateBackend::D3D12PipelineStateBackend() {

}

D3D12PipelineStateBackend::~D3D12PipelineStateBackend() {

}

PipelineStatePointer D3D12Backend::createPipelineState(const PipelineStateInit& init) {
    D3D12PipelineStateBackend* pso = nullptr;
    if (init.program) {

        auto vertexShader = static_cast<D3D12ShaderBackend*> (init.program->getVertexShader().get());
        auto pixelShader = static_cast<D3D12ShaderBackend*> (init.program->getPixelShader().get());
        ComPtr<ID3DBlob> vertexShaderBlob = vertexShader->_shaderBlob;
        ComPtr<ID3DBlob> pixelShaderBlob = pixelShader->_shaderBlob;

        ComPtr<ID3D12RootSignature> rootSignature;
        ComPtr<ID3D12PipelineState> pipelineState;

        // Create an empty root signature.
        {
            D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
            rootSignatureDesc.NumParameters = 0;
            rootSignatureDesc.pParameters = nullptr;
            rootSignatureDesc.NumStaticSamplers = 0;
            rootSignatureDesc.pStaticSamplers = nullptr;
            rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            ComPtr<ID3DBlob> signature;
            ComPtr<ID3DBlob> error;
            ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
            ThrowIfFailed(_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
        }
        
        {
            // Define the vertex input layout.
            D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }//,
             //   { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };

            // Describe and create the graphics pipeline state object (PSO).
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
            psoDesc.pRootSignature = rootSignature.Get();
            psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShaderBlob.Get()->GetBufferPointer()), vertexShaderBlob.Get()->GetBufferSize() };
            psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShaderBlob.Get()->GetBufferPointer()), pixelShaderBlob.Get()->GetBufferSize() };
            

            psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
            psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
            psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
            psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
            psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
            psoDesc.RasterizerState.DepthClipEnable = TRUE;
            psoDesc.RasterizerState.MultisampleEnable = FALSE;
            psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
            psoDesc.RasterizerState.ForcedSampleCount = 0;
            psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;


            psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
            psoDesc.BlendState.IndependentBlendEnable = FALSE;
            psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
            psoDesc.BlendState.RenderTarget[0].SrcBlend	= D3D12_BLEND_ONE;
            psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
            psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
            psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            psoDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
            psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
            psoDesc.DepthStencilState.DepthEnable = FALSE;
            psoDesc.DepthStencilState.StencilEnable = FALSE;
            psoDesc.SampleMask = UINT_MAX;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.SampleDesc.Count = 1;
            ThrowIfFailed(_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
        
        }

        pso = new D3D12PipelineStateBackend();
        pso->_program = init.program;
        pso->_pipelineState = pipelineState;
        pso->_rootSignature = rootSignature;

    }

    return PipelineStatePointer(pso);
}


#endif
