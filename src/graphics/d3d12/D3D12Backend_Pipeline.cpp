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
#define ThrowIfFailed(result) if (FAILED((result))) picoLog("D3D12Backend_Pipeline FAILED !!!");


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
        auto vertexShaderBlob = vertexShader->_shaderBlob;
        auto pixelShaderBlob = pixelShader->_shaderBlob;

        ComPtr<ID3D12RootSignature> rootSignature;
        ComPtr<ID3D12PipelineState> pipelineState;

        // Grab the root signature from the associated pipeline layout.
        if (pso->_rootSignature) {
            rootSignature = pso->_rootSignature;
        } else {
            picoAssert((bool) pso->_rootDescriptorLayout);
            auto dxDescLayout = static_cast<D3D12RootDescriptorLayoutBackend*> (pso->_rootDescriptorLayout.get());
            rootSignature = dxDescLayout->_rootSignature;
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
        auto computeShaderBlob = computeShader->_shaderBlob;

        ComPtr<ID3D12RootSignature> rootSignature;
        ComPtr<ID3D12PipelineState> pipelineState;

        // Grab the root signature from the associated pipeline layout.
        if (pso->_rootSignature) {
            rootSignature = pso->_rootSignature;
        }
        else if (init.rootDescriptorLayout) {
            auto dxDescLayout = static_cast<D3D12RootDescriptorLayoutBackend*> (init.rootDescriptorLayout.get());
            rootSignature = dxDescLayout->_rootSignature;
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
    else if (pso->getType() == PipelineType::RAYTRACING && pso->_program) {
        auto& init = pso->_raytracing;

        ComPtr<ID3D12RootSignature> globalRootSignature;
        ComPtr<ID3D12RootSignature> localRootSignature;
        ComPtr<ID3D12StateObject> pipelineState;


        // Grab the global root signature from the associated pipeline layout.
        if (pso->_rootSignature) {
            globalRootSignature = pso->_rootSignature;
        }
        else if (init.globalRootDescriptorLayout) {
            auto dxDescLayout = static_cast<D3D12RootDescriptorLayoutBackend*> (init.globalRootDescriptorLayout.get());
            globalRootSignature = dxDescLayout->_rootSignature;
        }

        // Grab the local root signature from the associated pipeline layout.
        if (pso->_localRootSignature) {
            localRootSignature = pso->_localRootSignature;
        }
        else if (init.localRootDescriptorLayout) {
            auto dxDescLayout = static_cast<D3D12RootDescriptorLayoutBackend*> (init.localRootDescriptorLayout.get());
            localRootSignature = dxDescLayout->_rootSignature;
        }


        // Create 7 subobjects that combine into a RTPSO:
        // Subobjects need to be associated with DXIL exports (i.e. shaders) either by way of default or explicit associations.
        // Default association applies to every exported shader entrypoint that doesn't have any of the same type of subobject associated with it.
        // This simple sample utilizes default shader association except for local root signature subobject
        // which has an explicit association specified purely for demonstration purposes.
        // 3 - DXIL library
        // 1 - Triangle hit group
        // 2 - Shader config
        // 2 - Local root signature and association
        // 1 - Global root signature
        // 1 - Pipeline config

        ID3D12StateObjectProperties* rtpsoInfo;

        // Define state subobjects for shaders, root signatures,
        // and configuration data.
        std::vector<D3D12_STATE_SUBOBJECT> subobjects;
        subobjects.resize(10);
        UINT index = 0;



        auto raytracingShader = static_cast<D3D12ShaderBackend*> (init.program->getProgramDesc().shaderLib.at("RAYTRACING").get());
 
        // Describe the DXIL Library entry point and name.
        D3D12_EXPORT_DESC rgs_exportDesc;
        rgs_exportDesc.Name = L"MyRaygenShader";
        rgs_exportDesc.ExportToRename = L"MyRaygenShader";
        rgs_exportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

        // Describe the DXIL library.
        D3D12_DXIL_LIBRARY_DESC rgs_libDesc = {};
        rgs_libDesc.DXILLibrary.BytecodeLength = raytracingShader->_shaderLibBlob->GetBufferSize();
        rgs_libDesc.DXILLibrary.pShaderBytecode = raytracingShader->_shaderLibBlob->GetBufferPointer();
        rgs_libDesc.NumExports = 1;
        rgs_libDesc.pExports = &rgs_exportDesc;

        // Describe the ray generation shader state subobject.
        D3D12_STATE_SUBOBJECT rgs = {};
        rgs.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        rgs.pDesc = &rgs_libDesc;
        subobjects[index] = rgs;
        index++;






        // Describe the DXIL Library entry point and name.
        D3D12_EXPORT_DESC chs_exportDesc;
        chs_exportDesc.Name = L"MyClosestHitShader";
        chs_exportDesc.ExportToRename = L"MyClosestHitShader";
        chs_exportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

        // Describe the DXIL library.
        D3D12_DXIL_LIBRARY_DESC chs_libDesc = {};
        chs_libDesc.DXILLibrary.BytecodeLength = raytracingShader->_shaderLibBlob->GetBufferSize();
        chs_libDesc.DXILLibrary.pShaderBytecode = raytracingShader->_shaderLibBlob->GetBufferPointer();
        chs_libDesc.NumExports = 1;
        chs_libDesc.pExports = &chs_exportDesc;

        // Describe the ray generation shader state subobject.
        D3D12_STATE_SUBOBJECT chs = {};
        chs.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        chs.pDesc = &chs_libDesc;
        subobjects[index] = chs;
        index++;






        // Describe the DXIL Library entry point and name.
        D3D12_EXPORT_DESC ms_exportDesc;
        ms_exportDesc.Name = L"MyMissShader";
        ms_exportDesc.ExportToRename = L"MyMissShader";
        ms_exportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

        // Describe the DXIL library.
        D3D12_DXIL_LIBRARY_DESC ms_libDesc = {};
        ms_libDesc.DXILLibrary.BytecodeLength = raytracingShader->_shaderLibBlob->GetBufferSize();
        ms_libDesc.DXILLibrary.pShaderBytecode = raytracingShader->_shaderLibBlob->GetBufferPointer();
        ms_libDesc.NumExports = 1;
        ms_libDesc.pExports = &ms_exportDesc;

        // Describe the ray generation shader state subobject.
        D3D12_STATE_SUBOBJECT ms = {};
        ms.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        ms.pDesc = &ms_libDesc;
        subobjects[index] = ms;
        index++;




        D3D12_HIT_GROUP_DESC hitGroupDesc = {};
        hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
        hitGroupDesc.ClosestHitShaderImport = L"MyClosestHitShader";
       // hitGroupDesc.AnyHitShaderImport = L"Unique_AHS_Name";
     //   hitGroupDesc.IntersectionShaderImport = L"Unique_IS_Name";
        hitGroupDesc.HitGroupExport = L"HitGroup_Name";

        // Describe the hit group state subobject.
        D3D12_STATE_SUBOBJECT hitGroup = {};
        hitGroup.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
        hitGroup.pDesc = &hitGroupDesc;
        subobjects[index] = hitGroup;
        index++;




        // Describe the shader payload configuration.
        D3D12_RAYTRACING_SHADER_CONFIG shdrConfigDesc = {};
        shdrConfigDesc.MaxPayloadSizeInBytes = 4 * sizeof(float);
        shdrConfigDesc.MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;

        D3D12_STATE_SUBOBJECT shdrConfig = {};
        shdrConfig.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
        shdrConfig.pDesc = &shdrConfigDesc;
        subobjects[index] = shdrConfig;
        index++;





        // Create a list of shader export names that use the payload.
        const WCHAR* payloadExports[] = { L"MyRaygenShader", L"MyMissShader", L"HitGroup_Name" };
        // Describe the association of shaders and a local root signature.
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION payload_assocDesc = {};
        payload_assocDesc.NumExports = _countof(payloadExports);
        payload_assocDesc.pExports = payloadExports;
        payload_assocDesc.pSubobjectToAssociate = &subobjects[index - 1];

        // Create the association subobject.
        D3D12_STATE_SUBOBJECT payload_association = {};
        payload_association.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        payload_association.pDesc = &payload_assocDesc;
        subobjects[index] = payload_association;
        index++;




  
        // Local root signature
        D3D12_LOCAL_ROOT_SIGNATURE L_R_S = {};
        L_R_S.pLocalRootSignature = localRootSignature.Get();

        D3D12_STATE_SUBOBJECT localRootSignatureSO = {};
        localRootSignatureSO.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
        localRootSignatureSO.pDesc = &L_R_S;
        subobjects[index] = localRootSignatureSO;
        index++;





        // Create a list of shader export names that use the root signature.
        const WCHAR* lrsExports[] = { L"MyRaygenShader", L"HitGroup_Name", L"MyMissShader" };
        // Describe the association of shaders and a local root signature.
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION lrs_assocDesc = {};
        lrs_assocDesc.NumExports = _countof(lrsExports);
        lrs_assocDesc.pExports = lrsExports;
        lrs_assocDesc.pSubobjectToAssociate = &subobjects[index - 1];

        // Create the association subobject.
        D3D12_STATE_SUBOBJECT lrs_association = {};
        lrs_association.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        lrs_association.pDesc = &lrs_assocDesc;
        subobjects[index] = lrs_association;
        index++;


        // Global root signature to be used in a ray gen shader.
        D3D12_GLOBAL_ROOT_SIGNATURE G_R_S = {};
        G_R_S.pGlobalRootSignature = globalRootSignature.Get();
        G_R_S.pGlobalRootSignature = nullptr;

        D3D12_STATE_SUBOBJECT globalRootSignatureSO = {};
        globalRootSignatureSO.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
        globalRootSignatureSO.pDesc = &G_R_S;
        subobjects[index] = globalRootSignatureSO;
        index++;



        
       // Describe the ray tracing pipeline configuration.
        D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfigDesc = {};
        pipelineConfigDesc.MaxTraceRecursionDepth = 1;

        D3D12_STATE_SUBOBJECT pipelineConfig = {};
        pipelineConfig.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
        pipelineConfig.pDesc = &pipelineConfigDesc;
        subobjects[index] = pipelineConfig;
        index++;



        // Create the state object.
        D3D12_STATE_OBJECT_DESC rtpsoDesc;
        rtpsoDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
        rtpsoDesc.NumSubobjects = index;
        rtpsoDesc.pSubobjects = subobjects.data();

     //   rtpsoDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
     //   rtpsoDesc.NumSubobjects = 0;
     //   rtpsoDesc.pSubobjects = nullptr;

        ThrowIfFailed(_device->CreateStateObject(&rtpsoDesc, IID_PPV_ARGS(&pipelineState)));


        // Get the ray tracing pipeline state object's properties.
        pipelineState->QueryInterface(IID_PPV_ARGS(&rtpsoInfo));


        // update these
        if (pso->_stateObject) {
            ComPtr<ID3D12DeviceChild> c;
            pso->_stateObject.As(&c);
            garbageCollect(c);
        }
        pso->_stateObject = pipelineState;
        if (!pso->_rootSignature) {
            pso->_rootSignature = globalRootSignature;
        }
        if (!pso->_localRootSignature) {
            pso->_localRootSignature = localRootSignature;
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
    pso->_rootDescriptorLayout = ( init.rootDescriptorLayout ? init.rootDescriptorLayout : _emptyRootDescriptorLayout); // assign empty pipeline layout if none specified

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
    pso->_rootDescriptorLayout = (init.rootDescriptorLayout ? init.rootDescriptorLayout : _emptyRootDescriptorLayout); // assign empty pipeline layout if none specified


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


PipelineStatePointer D3D12Backend::createRaytracingPipelineState(const RaytracingPipelineStateInit& init) {
    auto pso = std::make_shared<D3D12PipelineStateBackend>();
    pso->_type = PipelineType::RAYTRACING;
    pso->_raytracing = init;
    pso->_program = init.program;
    pso->_rootDescriptorLayout = (init.globalRootDescriptorLayout ? init.globalRootDescriptorLayout : _emptyRootDescriptorLayout); // assign empty pipeline layout if none specified
    pso->_localRootDescriptorLayout = (init.localRootDescriptorLayout ? init.localRootDescriptorLayout : _emptyRootDescriptorLayout); // assign empty pipeline layout if none specified


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
#define ALIGN(_alignment, _val) (((_val + _alignment - 1) / _alignment) * _alignment)

ShaderEntry D3D12Backend::getShaderEntry(const PipelineStatePointer& pipeline, const std::string& entryName) {
    auto pipelineState = static_cast<D3D12PipelineStateBackend*>(pipeline.get());

    // Get the  pipeline state object's properties.
    ID3D12StateObjectProperties* rtpsoInfo;
    pipelineState->_stateObject->QueryInterface(IID_PPV_ARGS(&rtpsoInfo));

    ShaderEntry entry;

    uint32_t shaderIdSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    uint32_t shaderTableSize = 0;
    uint32_t shaderTableRecordSize = shaderIdSize;
    shaderTableRecordSize += 8; // CBV/SRV/UAV descriptor table
    shaderTableRecordSize = ALIGN(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, shaderTableRecordSize);


    // Shader Record 0 - Ray Generation program and local root parameter data (descriptor table with constant buffer and IB/VB pointers)
    auto lEntryName = core::to_wstring(entryName);
    memcpy(entry._blob.data(), rtpsoInfo->GetShaderIdentifier(lEntryName.c_str()), shaderIdSize);


    return entry;
}


D3D12ShaderTableBackend::D3D12ShaderTableBackend()
{}

D3D12ShaderTableBackend::~D3D12ShaderTableBackend()
{

}

ShaderTablePointer D3D12Backend::createShaderTable(const ShaderTableInit& init) {
    auto shaderTable = std::make_shared<D3D12ShaderTableBackend>();


#define ALIGN(_alignment, _val) (((_val + _alignment - 1) / _alignment) * _alignment)

    /*
    The Shader Table layout is as follows:
        Entry 0 - Ray Generation shader
        Entry 1 - Miss shader
        Entry 2 - Closest Hit shader
    All shader records in the Shader Table must have the same size, so shader record size will be based on the largest required entry.
    The ray generation program requires the largest entry:
        32 bytes - D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
      +  8 bytes - a CBV/SRV/UAV descriptor table pointer (64-bits)
      = 40 bytes ->> aligns to 64 bytes
    The entry size must be aligned up to D3D12_RAYTRACING_SHADER_BINDING_TABLE_RECORD_BYTE_ALIGNMENT
    */
    uint32_t shaderIdSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    uint32_t shaderTableSize = 0;

    uint32_t shaderTableRecordSize = shaderIdSize;
    shaderTableRecordSize += 8;							// CBV/SRV/UAV descriptor table
    shaderTableRecordSize = ALIGN(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, shaderTableRecordSize);

    shaderTableSize = (shaderTableRecordSize * 3);		// 3 shader records in the table
    shaderTableSize = ALIGN(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT, shaderTableSize);


#define TO_DESC(x)(*reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(x))
    ComPtr<ID3D12Resource> shaderTableBuffer;
    ID3D12DescriptorHeap* heap;
    {
        D3D12_HEAP_PROPERTIES heapDesc = {};
        heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapDesc.CreationNodeMask = 1;
        heapDesc.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Width = shaderTableSize;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // Create the GPU resource
        HRESULT hr = _device->CreateCommittedResource(&heapDesc, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&shaderTableBuffer));
    }

    // Map the buffer
    uint8_t* pData;
    HRESULT hr = shaderTableBuffer->Map(0, nullptr, (void**)&pData);


    // Shader Record 0 - Ray Generation program and local root parameter data (descriptor table with constant buffer and IB/VB pointers)
    

    memcpy(pData, init.records[0].shaderEntry._blob.data(), shaderIdSize);

    auto gpuHandle = std::static_pointer_cast<D3D12DescriptorSetBackend>(init.records[0].descriptorSet)->_cbvsrvuav_GPUHandle;
    memcpy(pData + shaderIdSize, &gpuHandle, sizeof(gpuHandle));

    pData += shaderTableRecordSize;



    memcpy(pData, init.records[1].shaderEntry._blob.data(), shaderIdSize);

    gpuHandle = std::static_pointer_cast<D3D12DescriptorSetBackend>(init.records[1].descriptorSet)->_cbvsrvuav_GPUHandle;
    memcpy(pData + shaderIdSize, &gpuHandle, sizeof(gpuHandle));

    pData += shaderTableRecordSize;


    memcpy(pData, init.records[2].shaderEntry._blob.data(), shaderIdSize);

    gpuHandle = std::static_pointer_cast<D3D12DescriptorSetBackend>(init.records[2].descriptorSet)->_cbvsrvuav_GPUHandle;
    memcpy(pData + shaderIdSize, &gpuHandle, sizeof(gpuHandle));

    pData += shaderTableRecordSize;
/*
    struct LViewport
    {
        float left;
        float top;
        float right;
        float bottom;
    };

    struct RayGenConstantBuffer
    {
        LViewport viewport;
        LViewport stencil;
    };

    struct RootArguments {
        RayGenConstantBuffer cb;
    } rootArguments;
    //rootArguments.cb = m_rayGenCB;

    // Set the root parameter data. Point to start of descriptor heap.
    *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(pData + shaderIdSize) = _descriptorHeap->gpuHandle(allocatedDescriptors);

    // Shader Record 1 - Miss program (no local root arguments to set)
    pData += shaderTableRecordSize;
    memcpy(pData, init.records[1].shaderEntry._blob.data(), shaderIdSize);

    // Shader Record 2 - Closest Hit program and local root parameter data (descriptor table with constant buffer and IB/VB pointers)
    pData += shaderTableRecordSize;
    memcpy(pData, init.records[2].shaderEntry._blob.data(), shaderIdSize);

    // Set the root parameter data. Point to start of descriptor heap.
    *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(pData + shaderIdSize) = descriptorHeap->gpuHandle(allocatedDescriptors);
*/
    // Unmap
    shaderTableBuffer->Unmap(0, nullptr);
    shaderTable->_shaderTable = shaderTableBuffer;

    return shaderTable;
}

#endif
