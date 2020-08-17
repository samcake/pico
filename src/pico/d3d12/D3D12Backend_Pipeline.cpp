// D3D12Backend_Descriptor.cpp
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

using namespace pico;

#ifdef _WINDOWS
#define ThrowIfFailed(result) if (FAILED((result))) picoLog() << "FAILED !!!/n";


D3D12PipelineStateBackend::D3D12PipelineStateBackend() {

}

D3D12PipelineStateBackend::~D3D12PipelineStateBackend() {

}

PipelineStatePointer D3D12Backend::createPipelineState(const PipelineStateInit & init) {
    D3D12PipelineStateBackend* pso = nullptr;
    if (init.program) {

        auto vertexShader = static_cast<D3D12ShaderBackend*> (init.program->getVertexShader().get());
        auto pixelShader = static_cast<D3D12ShaderBackend*> (init.program->getPixelShader().get());
        ComPtr<ID3DBlob> vertexShaderBlob = vertexShader->_shaderBlob;
        ComPtr<ID3DBlob> pixelShaderBlob = pixelShader->_shaderBlob;

        ComPtr<ID3D12RootSignature> rootSignature;
        ComPtr<ID3D12PipelineState> pipelineState;

        // Create an empty root signature if none provided.
        if (init.descriptorSetLayout) {
            auto dxDescLayout = static_cast<D3D12DescriptorSetLayoutBackend*> (init.descriptorSetLayout.get());
            rootSignature = dxDescLayout->_rootSignature;
        } else {
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
            const std::string SemanticToName[int(AttribSemantic::COUNT)] = { "POSITION", "NORMAL", "COLOR" };
            const DXGI_FORMAT AttribFormatToFormat[int(AttribFormat::COUNT)] = { DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R8G8B8A8_UNORM };

            std::vector< D3D12_INPUT_ELEMENT_DESC > inputElementDescs(init.streamLayout.numAttribs());
            auto inputElement = inputElementDescs.data();
            for (int a = 0; a < init.streamLayout.numAttribs(); a++) {
                auto attrib = init.streamLayout.getAttrib(a);
                inputElement->SemanticName = SemanticToName[(int)attrib->_semantic].c_str();
                inputElement->SemanticIndex = 0;
                inputElement->Format = AttribFormatToFormat[(int)attrib->_format];
                inputElement->InputSlot = attrib->_bufferIndex;
                inputElement->AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
                inputElement->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                inputElement->InstanceDataStepRate = 0;

                inputElement++;
            }

            // Describe and create the graphics pipeline state object (PSO).
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.InputLayout = { inputElementDescs.data(), (uint32_t)inputElementDescs.size() };
            psoDesc.pRootSignature = rootSignature.Get();
            psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShaderBlob.Get()->GetBufferPointer()), vertexShaderBlob.Get()->GetBufferSize() };
            psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShaderBlob.Get()->GetBufferPointer()), pixelShaderBlob.Get()->GetBufferSize() };


            psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
            psoDesc.RasterizerState.FrontCounterClockwise = TRUE; // This is gltf convention front faces are CCW
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

            if (init.blend) {
                psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
            } else {
                psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
            }
            psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
            psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            
            psoDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
            psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            psoDesc.BlendState.AlphaToCoverageEnable = FALSE;

            if (init.depth) {
                psoDesc.DepthStencilState.DepthEnable = TRUE; // enable depth
                psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
                psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
                // ^ We re using Inverted Z so need to test to pass greater tahn because near is at 1 and far is at 0
            } else {
                psoDesc.DepthStencilState.DepthEnable = FALSE; // disable depth
                psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
                psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
            }

            psoDesc.DepthStencilState.StencilEnable = FALSE;
            psoDesc.DepthStencilState.StencilReadMask = 0xFF;
            psoDesc.DepthStencilState.StencilWriteMask = 0xFF;
            //            D3D12_DEPTH_STENCILOP_DESC FrontFace;
 //           D3D12_DEPTH_STENCILOP_DESC BackFace;

            psoDesc.SampleMask = UINT_MAX;
            psoDesc.PrimitiveTopologyType = D3D12BatchBackend::PrimitiveTopologyTypes[(int)init.primitiveTopology];
            psoDesc.NumRenderTargets = 1;
           // psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.SampleDesc.Count = 1;
            if (init.depth) {
                psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
            } else {
                psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
            }
            ThrowIfFailed(_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));

        }

        pso = new D3D12PipelineStateBackend();
        pso->_init = init;
        pso->_program = init.program;
        pso->_pipelineState = pipelineState;
        pso->_rootSignature = rootSignature;

        pso->_primitive_topology = D3D12BatchBackend::PrimitiveTopologies[(int)init.primitiveTopology];
    }

    return PipelineStatePointer(pso);
}



D3D12SamplerBackend::D3D12SamplerBackend() {

}

D3D12SamplerBackend::~D3D12SamplerBackend() {

}

SamplerPointer D3D12Backend::createSampler(const SamplerInit& init) {
    auto sampler = new D3D12SamplerBackend;
    sampler->_samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler->_samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler->_samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler->_samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler->_samplerDesc.MipLODBias = 0;
    sampler->_samplerDesc.MaxAnisotropy = 0;
    sampler->_samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler->_samplerDesc.BorderColor[0] = 0.0f;
    sampler->_samplerDesc.BorderColor[1] = 0.0f;
    sampler->_samplerDesc.BorderColor[2] = 0.0f;
    sampler->_samplerDesc.BorderColor[3] = 0.0f;
    sampler->_samplerDesc.MinLOD = 0.0f;
    sampler->_samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

    return SamplerPointer(sampler);
}


#endif
