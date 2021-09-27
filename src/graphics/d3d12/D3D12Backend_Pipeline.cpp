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

using namespace graphics;

#ifdef _WINDOWS
#define ThrowIfFailed(result) if (FAILED((result))) picoLog() << "FAILED !!!/n";


D3D12PipelineStateBackend::D3D12PipelineStateBackend() {

}

D3D12PipelineStateBackend::~D3D12PipelineStateBackend() {

}

bool D3D12Backend::realizePipelineState(PipelineState* pipeline) {
    D3D12PipelineStateBackend* pso = dynamic_cast<D3D12PipelineStateBackend*> (pipeline);

    if (pso->getType() == PipelineType::GRAPHICS && pso->_program) {
        auto& init = pso->_graphics;

        auto vertexShader = static_cast<D3D12ShaderBackend*> (init.program->getVertexShader().get());
        auto pixelShader = static_cast<D3D12ShaderBackend*> (init.program->getPixelShader().get());
        ComPtr<ID3DBlob> vertexShaderBlob = vertexShader->_shaderBlob;
        ComPtr<ID3DBlob> pixelShaderBlob = pixelShader->_shaderBlob;

        ComPtr<ID3D12RootSignature> rootSignature;
        ComPtr<ID3D12PipelineState> pipelineState;

        // Create an empty root signature if none provided.
        if (pso->_rootSignature) {
            rootSignature = pso->_rootSignature;
        } else if (init.rootDescriptorLayout) {
            auto dxDescLayout = static_cast<D3D12RootDescriptorLayoutBackend*> (init.rootDescriptorLayout.get());
            rootSignature = dxDescLayout->_rootSignature;
        }
        else {
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

            D3D12PipelineStateBackend::fill_rasterizer_desc(init.rasterizer, psoDesc.RasterizerState);
      
            D3D12PipelineStateBackend::fill_depth_stencil_desc(init.depthStencil, psoDesc.DepthStencilState);

            D3D12PipelineStateBackend::fill_blend_desc(init.blend, psoDesc.BlendState);


            psoDesc.SampleMask = UINT_MAX;
            psoDesc.PrimitiveTopologyType = D3D12BatchBackend::PrimitiveTopologyTypes[(int)init.primitiveTopology];
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = D3D12Backend::Format[(uint32_t) init.colorTargetFormat];

            psoDesc.SampleDesc.Count = 1;
            if (init.depthStencil.depthTest.isEnabled()) {
                psoDesc.DSVFormat = D3D12Backend::Format[(uint32_t)init.depthStencilFormat];
            }
            else {
                psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
            }
            ThrowIfFailed(_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));

            pso->_primitive_topology = D3D12BatchBackend::PrimitiveTopologies[(int)init.primitiveTopology];
        }


        // update these
        if (pso->_pipelineState) {
            ComPtr<ID3D12DeviceChild> c;
            pso->_pipelineState.As(&c);
            garbageCollect(c);
        }
        pso->_pipelineState = pipelineState;
        if (!pso->_rootSignature) {
            pso->_rootSignature = rootSignature;
        }


        return true;
    }
    else if (pso->getType() == PipelineType::COMPUTE && pso->_program) {
        auto& init = pso->_compute;


        auto computeShader = static_cast<D3D12ShaderBackend*> (init.program.get());
        ComPtr<ID3DBlob> computeShaderBlob = computeShader->_shaderBlob;

        ComPtr<ID3D12RootSignature> rootSignature;
        ComPtr<ID3D12PipelineState> pipelineState;

        // Create an empty root signature if none provided.
        if (pso->_rootSignature) {
            rootSignature = pso->_rootSignature;
        }
        else if (init.rootDescriptorLayout) {
            auto dxDescLayout = static_cast<D3D12RootDescriptorLayoutBackend*> (init.rootDescriptorLayout.get());
            rootSignature = dxDescLayout->_rootSignature;
        }
        else {
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

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = rootSignature.Get();


        psoDesc.CS = { reinterpret_cast<UINT8*>(computeShaderBlob.Get()->GetBufferPointer()), computeShaderBlob.Get()->GetBufferSize() };

        ThrowIfFailed(_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));

        // update these
        if (pso->_pipelineState) {
            ComPtr<ID3D12DeviceChild> c;
            pso->_pipelineState.As(&c);
            garbageCollect(c);
        }
        pso->_pipelineState = pipelineState;
        if (!pso->_rootSignature) {
            pso->_rootSignature = rootSignature;
        }


        return true;
    }
    
    return false;
}

PipelineStatePointer D3D12Backend::createGraphicsPipelineState(const GraphicsPipelineStateInit & init) {
    auto pso = std::make_shared<D3D12PipelineStateBackend>();
    pso->_type = PipelineType::GRAPHICS;
    pso->_graphics = init;
    pso->_program = init.program;
    pso->_rootDescriptorLayout = init.rootDescriptorLayout;

    if (realizePipelineState(pso.get())) {

        if (pso->_program->hasWatcher()) {
            auto pipelineRealizer = std::function([&](PipelineState* pipeline) -> bool {
                return realizePipelineState(pipeline);
            });

            PipelineState::registerToWatcher(pso, pipelineRealizer);
        }

        return pso;
    }

    return nullptr;
}

PipelineStatePointer D3D12Backend::createComputePipelineState(const ComputePipelineStateInit& init) {
    auto pso = std::make_shared<D3D12PipelineStateBackend>();
    pso->_type = PipelineType::COMPUTE;
    pso->_compute = init;
    pso->_program = init.program;
    pso->_rootDescriptorLayout = init.rootDescriptorLayout;


    if (realizePipelineState(pso.get())) {

        if (pso->_program->hasWatcher()) {
            auto pipelineRealizer = std::function([&](PipelineState* pipeline) -> bool {
                return realizePipelineState(pipeline);
                });

            PipelineState::registerToWatcher(pso, pipelineRealizer);
        }

        return pso;
    }

    return nullptr;
}

D3D12SamplerBackend::D3D12SamplerBackend() {

}

D3D12SamplerBackend::~D3D12SamplerBackend() {

}

SamplerPointer D3D12Backend::createSampler(const SamplerInit& init) {

    const D3D12_FILTER Filter_to_d3d12[]{
        D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR,
        D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
        D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR,
        D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT,
        D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_FILTER_ANISOTROPIC,
        D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
        D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
        D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
        D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
        D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
        D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
        D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
        D3D12_FILTER_COMPARISON_ANISOTROPIC,
        D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT,
        D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR,
        D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
        D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR,
        D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT,
        D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT,
        D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR,
        D3D12_FILTER_MINIMUM_ANISOTROPIC,
        D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT,
        D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR,
        D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
        D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR,
        D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT,
        D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT,
        D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR,
        D3D12_FILTER_MAXIMUM_ANISOTROPIC
    };
    const D3D12_TEXTURE_ADDRESS_MODE AddressMode_to_d3d12[]{
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE
    };

    auto sampler = new D3D12SamplerBackend;
    sampler->_samplerDesc.Filter = Filter_to_d3d12[(int32_t)init._filter];
    sampler->_samplerDesc.AddressU = AddressMode_to_d3d12[(int32_t)init._addressU];
    sampler->_samplerDesc.AddressV = AddressMode_to_d3d12[(int32_t)init._addressV];
    sampler->_samplerDesc.AddressW = AddressMode_to_d3d12[(int32_t)init._addressW];
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

void D3D12PipelineStateBackend::fill_rasterizer_desc(const RasterizerState& src, D3D12_RASTERIZER_DESC& dst) {

    dst.FillMode = D3D12_FILL_MODE_SOLID;
    const D3D12_FILL_MODE FillMode_to_d3d12[]{
        D3D12_FILL_MODE_SOLID,
        D3D12_FILL_MODE_WIREFRAME,
        D3D12_FILL_MODE_WIREFRAME
    };
    dst.FillMode = FillMode_to_d3d12[(int8_t)src.fillMode];
    dst.FillMode = D3D12_FILL_MODE_SOLID;

    const D3D12_CULL_MODE CullMode_to_d3d12[]{
        D3D12_CULL_MODE_NONE,
        D3D12_CULL_MODE_FRONT,
        D3D12_CULL_MODE_BACK
    };
    dst.CullMode = CullMode_to_d3d12[(int8_t)src.cullMode];

    dst.FrontCounterClockwise = !src.frontFaceClockwise; // This is gltf convention front faces are CCW

    dst.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    dst.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    dst.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;

    dst.DepthClipEnable = src.depthClampEnable;
    dst.MultisampleEnable = src.multisampleEnable;
    dst.AntialiasedLineEnable = src.antialiasedLineEnable;
    dst.ForcedSampleCount = 0;

    dst.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

}

void D3D12PipelineStateBackend::fill_depth_stencil_desc(const DepthStencilState& src, D3D12_DEPTH_STENCIL_DESC& dst) {


    const D3D12_COMPARISON_FUNC ComparisonFunction_to_d3d12[]{
        D3D12_COMPARISON_FUNC_NEVER,
        D3D12_COMPARISON_FUNC_LESS,
        D3D12_COMPARISON_FUNC_EQUAL,
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_COMPARISON_FUNC_GREATER,
        D3D12_COMPARISON_FUNC_NOT_EQUAL,
        D3D12_COMPARISON_FUNC_GREATER_EQUAL,
        D3D12_COMPARISON_FUNC_ALWAYS
    };

    if (src.depthTest.isEnabled()) {
        dst.DepthEnable = TRUE; // enable depth
        dst.DepthWriteMask = (src.depthTest.isWriteEnabled() ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO);
        dst.DepthFunc = ComparisonFunction_to_d3d12[(int16_t)src.depthTest.getFunction()];//D3D12_COMPARISON_FUNC_GREATER;
        // ^ We re using Inverted Z so need to test to pass greater tahn because near is at 1 and far is at 0
    }
    else {
        dst.DepthEnable = FALSE; // disable depth
        dst.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        dst.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    }

    const D3D12_STENCIL_OP StencilOp_to_d3d12[]{
        D3D12_STENCIL_OP_ZERO,
        D3D12_STENCIL_OP_REPLACE,
        D3D12_STENCIL_OP_INCR_SAT,
        D3D12_STENCIL_OP_DECR_SAT,
        D3D12_STENCIL_OP_INVERT,
        D3D12_STENCIL_OP_INCR,
        D3D12_STENCIL_OP_DECR,
    };

    if (src.stencilActivation.isEnabled()) {
        dst.StencilEnable = TRUE;
        // D3D12 has only one value for read and write mask regardless of the side
        // let's use front
        dst.StencilReadMask = src.stencilTestFront.getReadMask(); 
        dst.StencilWriteMask = src.stencilActivation.getWriteMaskFront();

        dst.FrontFace.StencilFunc = ComparisonFunction_to_d3d12[(int16_t)src.stencilTestFront.getFunction()];
        dst.FrontFace.StencilFailOp = StencilOp_to_d3d12[(int16_t)src.stencilTestFront.getFailOp()];
        dst.FrontFace.StencilDepthFailOp = StencilOp_to_d3d12[(int16_t)src.stencilTestFront.getDepthFailOp()];
        dst.FrontFace.StencilPassOp = StencilOp_to_d3d12[(int16_t)src.stencilTestFront.getPassOp()];

        dst.BackFace.StencilFunc = ComparisonFunction_to_d3d12[(int16_t)src.stencilTestBack.getFunction()];
        dst.BackFace.StencilFailOp = StencilOp_to_d3d12[(int16_t)src.stencilTestBack.getFailOp()];
        dst.BackFace.StencilDepthFailOp = StencilOp_to_d3d12[(int16_t)src.stencilTestBack.getDepthFailOp()];
        dst.BackFace.StencilPassOp = StencilOp_to_d3d12[(int16_t)src.stencilTestBack.getPassOp()];
    } else {
        dst.StencilEnable = FALSE;
        dst.StencilReadMask = 0xFF;
        dst.StencilWriteMask = 0xFF;
    }
}

void D3D12PipelineStateBackend::fill_blend_desc(const BlendState& src, D3D12_BLEND_DESC& dst) {
    const D3D12_BLEND BlendArg_to_d3d12[] {
        D3D12_BLEND_ZERO,
        D3D12_BLEND_ONE,
        D3D12_BLEND_SRC_COLOR,
        D3D12_BLEND_INV_SRC_COLOR,
        D3D12_BLEND_SRC_ALPHA,
        D3D12_BLEND_INV_SRC_ALPHA,
        D3D12_BLEND_DEST_ALPHA,
        D3D12_BLEND_INV_DEST_ALPHA,
        D3D12_BLEND_DEST_COLOR,
        D3D12_BLEND_INV_DEST_COLOR,
        D3D12_BLEND_SRC_ALPHA_SAT,
        D3D12_BLEND_SRC1_COLOR, //D3D12_BLEND_BLEND_FACTOR,
        D3D12_BLEND_INV_SRC1_COLOR, //D3D12_BLEND_INV_BLEND_FACTOR,
        D3D12_BLEND_SRC1_ALPHA,
        D3D12_BLEND_INV_SRC1_ALPHA
    };

    const D3D12_BLEND_OP BlendOp_to_d3d12[]{
        D3D12_BLEND_OP_ADD,
        D3D12_BLEND_OP_SUBTRACT,
        D3D12_BLEND_OP_REV_SUBTRACT,
        D3D12_BLEND_OP_MIN,
        D3D12_BLEND_OP_MAX
    };



    if (src.blendFunc.isEnabled()) {
        dst.RenderTarget[0].BlendEnable = TRUE;
        dst.RenderTarget[0].SrcBlend = BlendArg_to_d3d12[(int16_t)src.blendFunc.getSourceColor()];
        dst.RenderTarget[0].DestBlend = BlendArg_to_d3d12[(int16_t)src.blendFunc.getDestinationColor()];
        dst.RenderTarget[0].BlendOp = BlendOp_to_d3d12[(int16_t)src.blendFunc.getOperationColor()];

        dst.RenderTarget[0].SrcBlendAlpha = BlendArg_to_d3d12[(int16_t)src.blendFunc.getSourceAlpha()];
        dst.RenderTarget[0].DestBlendAlpha = BlendArg_to_d3d12[(int16_t)src.blendFunc.getDestinationAlpha()];
        dst.RenderTarget[0].BlendOpAlpha = BlendOp_to_d3d12[(int16_t)src.blendFunc.getOperationAlpha()];


    } else {
        dst.RenderTarget[0].BlendEnable = FALSE;
        dst.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        dst.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
        dst.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        dst.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        dst.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        dst.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    }

    dst.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;


    // enum D3D12_COLOR_WRITE_ENABLE
    // {
    //     D3D12_COLOR_WRITE_ENABLE_RED = 1,
    //     D3D12_COLOR_WRITE_ENABLE_GREEN = 2,
    //     D3D12_COLOR_WRITE_ENABLE_BLUE = 4,
    //     D3D12_COLOR_WRITE_ENABLE_ALPHA = 8,
    //     D3D12_COLOR_WRITE_ENABLE_ALL = (((D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN) | D3D12_COLOR_WRITE_ENABLE_BLUE) | D3D12_COLOR_WRITE_ENABLE_ALPHA)
    // }
    // same bitfields meanings for the graphics::color_mask enum
    dst.RenderTarget[0].RenderTargetWriteMask = src.colorWriteMask;


    dst.AlphaToCoverageEnable = FALSE;

}


#endif
