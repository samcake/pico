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

#include <vector>

using namespace graphics;

#ifdef _WINDOWS
#define ThrowIfFailed(result) if (FAILED((result))) picoLog() << "FAILED !!!/n";

D3D12RootDescriptorLayoutBackend::D3D12RootDescriptorLayoutBackend() {

}

D3D12RootDescriptorLayoutBackend::~D3D12RootDescriptorLayoutBackend() {

}

D3D12_SHADER_VISIBILITY EvalShaderVisibility(ShaderStage _shaderStage) {
    // Start out with visibility on all shader stages
    D3D12_SHADER_VISIBILITY ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    uint32_t shader_stage_count = 0;
    // Select one if there is only one
    int shaderStage = (int)_shaderStage;
    if (shaderStage & ((int)ShaderStage::VERTEX)) {
        ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        ++shader_stage_count;
    }
    /*        if (shaderStage & ((int)ShaderStage::HULL)) {
                param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_HULL;
                param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_HULL;
                ++shader_stage_count;
            }
            if (shaderStage & ((int)ShaderStage::DOMAIN)) {
                param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN;
                param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN;
                ++shader_stage_count;
            }
            if (shaderStage & ((int)ShaderStage::GEOMETRY)) {
                param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
                param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
                ++shader_stage_count;
            }*/
    if (shaderStage & ((int)ShaderStage::PIXEL)) {
        ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        ++shader_stage_count;
    }
    if (shaderStage & ((int)ShaderStage::COMPUTE)) {
        // Keep D3D12_SHADER_VISIBILITY_ALL for compute shaders
        ++shader_stage_count;
    }

    // Go back to all shader stages if there's more than one stage
    if (shader_stage_count > 1) {
        ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }

    return ShaderVisibility;
 }

RootDescriptorLayoutPointer D3D12Backend::createRootDescriptorLayout(const RootDescriptorLayoutInit& init) {
    if (init._pushLayout.empty() && init._samplerLayout.empty() && init._setLayouts.empty()) {
        return nullptr;
    }

    auto descriptorLayout = new D3D12RootDescriptorLayoutBackend();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData;
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    HRESULT hres = _device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));
    if (FAILED(hres)) {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    uint32_t numDescriptors = 0;
    uint32_t numPushs = 0;
    uint32_t numSamplers = 0;
    struct DXSetDesc {
        uint32_t count = 0;
        std::vector<uint32_t> desc_offsets;
    };

    std::vector<DXSetDesc> descriptor_setDescs;
    for (const auto& dsl : init._setLayouts) {
        uint32_t numDescs = dsl.size();
        DXSetDesc setDesc;
        for (uint32_t i = 0; i < numDescs; ++i) {
            uint32_t count = dsl[i]._count;
            setDesc.count += count;
            setDesc.desc_offsets.emplace_back(i);
            numDescriptors++;
        }
        descriptor_setDescs.emplace_back(std::move(setDesc));
    }

    DXSetDesc sampler_setDesc;
    {
        const auto& dsl = init._samplerLayout;
        uint32_t numDescs = dsl.size();
        for (uint32_t i = 0; i < numDescs; ++i) {
            uint32_t count = dsl[i]._count;
            sampler_setDesc.count += count;
            sampler_setDesc.desc_offsets.emplace_back(i);
            numSamplers++;
        }
    }

    DXSetDesc push_setDesc;
    {
        const auto& dsl = init._pushLayout;
        uint32_t numDescs = dsl.size();
        for (uint32_t i = 0; i < numDescs; ++i) {
            uint32_t count = dsl[i]._count;
            push_setDesc.count += count;
            push_setDesc.desc_offsets.emplace_back(i);
            numPushs++;
        }
    }


    // Allocate the param indices (one per descriptor)
    descriptorLayout->_init = init;
    descriptorLayout->_dxPushParamIndices.resize(numPushs);
    descriptorLayout->_dxSetParamIndices.resize(numDescriptors);
    descriptorLayout->_dxSamplerParamIndices.resize(numSamplers);
    descriptorLayout->push_count = push_setDesc.count;
    descriptorLayout->sampler_count = sampler_setDesc.count;
    descriptorLayout->cbvsrvuav_count = (!numDescriptors ? 0 : descriptor_setDescs[0].count);

    // Allocate everything with an upper bound of descriptor counts
    uint32_t range_count = 0;
    uint32_t parameter_count = 0;

    auto ranges_11 = std::vector<D3D12_DESCRIPTOR_RANGE1>(numDescriptors + numSamplers + numPushs);
    auto ranges_10 = std::vector<D3D12_DESCRIPTOR_RANGE>(numDescriptors + numSamplers + numPushs);

    auto parameters_11 = std::vector<D3D12_ROOT_PARAMETER1>(numDescriptors + numSamplers + numPushs);
    auto parameters_10 = std::vector<D3D12_ROOT_PARAMETER>(numDescriptors + numSamplers + numPushs);

    // Build ranges
    for (uint32_t descriptor_index = 0; descriptor_index < numPushs; ++descriptor_index) {
        const auto& descriptor = init._pushLayout[descriptor_index];
        D3D12_DESCRIPTOR_RANGE1* range_11 = &ranges_11[range_count];
        D3D12_DESCRIPTOR_RANGE* range_10 = &ranges_10[range_count];
        D3D12_ROOT_PARAMETER1* param_11 = &parameters_11[parameter_count];
        D3D12_ROOT_PARAMETER* param_10 = &parameters_10[parameter_count];
        param_11->ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param_10->ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;

        param_10->ShaderVisibility = param_11->ShaderVisibility = EvalShaderVisibility(descriptor._shaderStage);

         // case DescriptorType::PUSH_UNIFORM: {
             param_11->Constants.RegisterSpace = 0;
             param_11->Constants.Num32BitValues = descriptor._count;
             param_11->Constants.ShaderRegister = descriptor._binding;
             param_10->Constants.RegisterSpace = 0;
             param_10->Constants.Num32BitValues = descriptor._count;
             param_10->Constants.ShaderRegister = descriptor._binding;
        // }

        descriptorLayout->_dxPushParamIndices[descriptor_index] = parameter_count;
        ++parameter_count;
    }

    for (uint32_t descriptor_index = 0; descriptor_index < numDescriptors; ++descriptor_index) {
        const auto& descriptor = init._setLayouts[0][descriptor_index];
        D3D12_DESCRIPTOR_RANGE1* range_11 = &ranges_11[range_count];
        D3D12_DESCRIPTOR_RANGE* range_10 = &ranges_10[range_count];
        D3D12_ROOT_PARAMETER1* param_11 = &parameters_11[parameter_count];
        D3D12_ROOT_PARAMETER* param_10 = &parameters_10[parameter_count];
        param_11->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param_10->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        param_10->ShaderVisibility = param_11->ShaderVisibility = EvalShaderVisibility(descriptor._shaderStage);

        switch (descriptor._type) {
        case DescriptorType::RESOURCE_BUFFER:
        case DescriptorType::RESOURCE_TEXTURE: {
            range_11->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            range_10->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        }
        break;
        case DescriptorType::UNIFORM_BUFFER: {
            range_11->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            range_10->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        }
        break;
        case DescriptorType::RW_RESOURCE_BUFFER:
        case DescriptorType::RW_RESOURCE_TEXTURE: {
            range_11->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            range_10->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        }
        break;
        }

        {
            range_11->NumDescriptors = descriptor._count;
            range_11->BaseShaderRegister = descriptor._binding;
            range_11->RegisterSpace = 0;
            range_11->Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            range_11->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

            range_10->NumDescriptors = descriptor._count;
            range_10->BaseShaderRegister = descriptor._binding;
            range_10->RegisterSpace = 0;
            range_10->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

            param_11->DescriptorTable.pDescriptorRanges = range_11;
            param_11->DescriptorTable.NumDescriptorRanges = 1;

            param_10->DescriptorTable.pDescriptorRanges = range_10;
            param_10->DescriptorTable.NumDescriptorRanges = 1;
            ++range_count;
        }

        descriptorLayout->_dxSetParamIndices[descriptor_index] = parameter_count;
        ++parameter_count;
    }


    for (uint32_t descriptor_index = 0; descriptor_index < numSamplers; ++descriptor_index) {
        const auto& descriptor = init._samplerLayout[descriptor_index];
        D3D12_DESCRIPTOR_RANGE1* range_11 = &ranges_11[range_count];
        D3D12_DESCRIPTOR_RANGE* range_10 = &ranges_10[range_count];
        D3D12_ROOT_PARAMETER1* param_11 = &parameters_11[parameter_count];
        D3D12_ROOT_PARAMETER* param_10 = &parameters_10[parameter_count];
        param_11->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param_10->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        param_10->ShaderVisibility = param_11->ShaderVisibility = EvalShaderVisibility(descriptor._shaderStage);

        // case DescriptorType::SAMPLER: {
        range_11->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        range_10->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        // }

        range_11->NumDescriptors = descriptor._count;
        range_11->BaseShaderRegister = descriptor._binding;
        range_11->RegisterSpace = 0;
        range_11->Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
        range_11->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        range_10->NumDescriptors = descriptor._count;
        range_10->BaseShaderRegister = descriptor._binding;
        range_10->RegisterSpace = 0;
        range_10->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        param_11->DescriptorTable.pDescriptorRanges = range_11;
        param_11->DescriptorTable.NumDescriptorRanges = 1;

        param_10->DescriptorTable.pDescriptorRanges = range_10;
        param_10->DescriptorTable.NumDescriptorRanges = 1;
        ++range_count;

        // }

        descriptorLayout->_dxSamplerParamIndices[descriptor_index] = parameter_count;
        ++parameter_count;
    }

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc;
    if (D3D_ROOT_SIGNATURE_VERSION_1_1 == featureData.HighestVersion) {
        desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        desc.Desc_1_1.NumParameters = parameter_count;
        desc.Desc_1_1.pParameters = parameters_11.data();
        desc.Desc_1_1.NumStaticSamplers = 0;
        desc.Desc_1_1.pStaticSamplers = NULL;
        desc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    }
    else if (D3D_ROOT_SIGNATURE_VERSION_1_0 == featureData.HighestVersion) {
        desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
        desc.Desc_1_0.NumParameters = parameter_count;
        desc.Desc_1_0.pParameters = parameters_10.data();
        desc.Desc_1_0.NumStaticSamplers = 0;
        desc.Desc_1_0.pStaticSamplers = NULL;
        desc.Desc_1_0.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    }

    ID3DBlob* sig_blob = NULL;
    ID3DBlob* error_msgs = NULL;
    if (D3D_ROOT_SIGNATURE_VERSION_1_1 == featureData.HighestVersion) {
        hres = fnD3D12SerializeVersionedRootSignature(&desc, &sig_blob, &error_msgs);
    }
    else {
        hres = D3D12SerializeRootSignature(&(desc.Desc_1_0), D3D_ROOT_SIGNATURE_VERSION_1_0, &sig_blob, &error_msgs);
    }
    picoAssert(SUCCEEDED(hres));
    if (error_msgs) {
        picoLog() << (char*)error_msgs->GetBufferPointer();
    }

    hres = _device->CreateRootSignature(0, sig_blob->GetBufferPointer(), sig_blob->GetBufferSize(),
        __uuidof(descriptorLayout->_rootSignature), (void**)&(descriptorLayout->_rootSignature));
    picoAssert(SUCCEEDED(hres));

    if (sig_blob) sig_blob->Release();
    if (error_msgs) error_msgs->Release();

    return RootDescriptorLayoutPointer(descriptorLayout);
}


D3D12DescriptorSetBackend::D3D12DescriptorSetBackend() {

}

D3D12DescriptorSetBackend::~D3D12DescriptorSetBackend() {

}

DescriptorSetPointer D3D12Backend::createDescriptorSet(const DescriptorSetInit& init) {

    auto descriptorSet = new D3D12DescriptorSetBackend();
    descriptorSet->_init = init;

    auto rootLayout = static_cast<const D3D12RootDescriptorLayoutBackend*>(init._rootLayout.get());
    auto rootSlot = init._slot;
    uint32_t numSetDescriptors = (uint32_t)rootLayout->_dxSetParamIndices.size();
    uint32_t numSamplerDescriptors = (uint32_t) rootLayout->_dxSamplerParamIndices.size();
    uint32_t cbvsrvuav_count = rootLayout->cbvsrvuav_count;
    uint32_t sampler_count = rootLayout->sampler_count;
    auto allocatedDescriptors = _descriptorHeap->allocateDescriptors(cbvsrvuav_count);
    auto allocatedSamplers = _descriptorHeap->allocateSamplers(sampler_count);
    auto descriptorHeap = static_cast<D3D12DescriptorHeapBackend*> (_descriptorHeap.get());
    // Assign heap offsets
    uint32_t cbvsrvuav_heap_offset = allocatedDescriptors;
    uint32_t sampler_heap_offset = allocatedSamplers;

    descriptorSet->_descriptorOffset = cbvsrvuav_heap_offset;
    descriptorSet->_samplerOffset = sampler_heap_offset;

    // TODO: THis will need to change ^ 
   
    // for each descriptor we need an offset even if not used:
    descriptorSet->_dxHeapOffsets.resize(numSetDescriptors + numSamplerDescriptors, 0);

    descriptorSet->_dxGPUHandles.resize(numSetDescriptors + numSamplerDescriptors);
    descriptorSet->_dxRootParameterIndices.resize(numSetDescriptors + numSamplerDescriptors, -1);

    for (uint32_t i = 0; i < numSetDescriptors; ++i) {
        descriptorSet->_dxHeapOffsets[i] = cbvsrvuav_heap_offset;
        descriptorSet->_dxGPUHandles[i] = descriptorHeap->gpuHandle(descriptorSet->_dxHeapOffsets[i]);
        descriptorSet->_dxRootParameterIndices[i] = rootLayout->_dxSetParamIndices[i];
        cbvsrvuav_heap_offset += rootLayout->_init._setLayouts[0][i]._count;
    }


    for (uint32_t i = 0; i < numSamplerDescriptors; ++i) {
        uint32_t is = i + numSetDescriptors;
        descriptorSet->_dxHeapOffsets[is] = sampler_heap_offset;
        descriptorSet->_dxGPUHandles[is] = descriptorHeap->gpuSamplerHandle(sampler_heap_offset);
        descriptorSet->_dxRootParameterIndices[is] = rootLayout->_dxSamplerParamIndices[i];
        sampler_heap_offset += rootLayout->_init._samplerLayout[i]._count;
    }



    descriptorSet->cbvsrvuav_count = cbvsrvuav_count;
    descriptorSet->sampler_count = sampler_count;

    descriptorSet->_numDescriptors = cbvsrvuav_count + sampler_count;


    return DescriptorSetPointer(descriptorSet);
}


void D3D12Backend::updateDescriptorSet(DescriptorSetPointer& descriptorSet, DescriptorObjects& objects) {
    auto dxDescriptorSet = static_cast<D3D12DescriptorSetBackend*> (descriptorSet.get());
    auto dxDescriptorHeap = static_cast<D3D12DescriptorHeapBackend*>(_descriptorHeap.get());

    
    uint32_t write_count = (uint32_t) objects.size();
   // Bail if there's nothing to write
    if (0 == write_count) {
        return;
    }

    if (write_count != dxDescriptorSet->cbvsrvuav_count + dxDescriptorSet->sampler_count) {
        return;
    }

    auto descriptorHeapOffset = dxDescriptorSet->_dxHeapOffsets[0];
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = dxDescriptorHeap->cpuHandle(dxDescriptorSet->_descriptorOffset);
    D3D12_CPU_DESCRIPTOR_HANDLE cpuSamplerHandle = dxDescriptorHeap->cpuSamplerHandle(dxDescriptorSet->_samplerOffset);

    for (uint32_t objectId = 0; objectId < objects.size(); ++objectId) {
        auto& descriptorObject = objects[objectId];

        switch (descriptorObject._type) {
        case DescriptorType::SAMPLER: {
            auto dxSampler = static_cast<D3D12SamplerBackend*> (descriptorObject._sampler.get());

            D3D12_SAMPLER_DESC* sampler_desc = &(dxSampler->_samplerDesc);
            _device->CreateSampler(sampler_desc, cpuSamplerHandle);

            cpuSamplerHandle.ptr += dxDescriptorHeap->_sampler_increment_size;
        } break;

        case DescriptorType::UNIFORM_BUFFER: {
            auto dxUbo = static_cast<D3D12BufferBackend*> (descriptorObject._buffer.get());

            ID3D12Resource* resource = dxUbo->_resource.Get();
            D3D12_CONSTANT_BUFFER_VIEW_DESC* view_desc = &(dxUbo->_uniformBufferView);
            _device->CreateConstantBufferView(view_desc, cpuHandle);

            cpuHandle.ptr += dxDescriptorHeap->_descriptor_increment_size;
        } break;

        case DescriptorType::RESOURCE_BUFFER: {
            auto dxBuffer = static_cast<D3D12BufferBackend*> (descriptorObject._buffer.get());
            if (dxBuffer) {
                ID3D12Resource* resource = dxBuffer->_resource.Get();
                D3D12_SHADER_RESOURCE_VIEW_DESC* view_desc = &(dxBuffer->_resourceBufferView);
                _device->CreateShaderResourceView(resource, view_desc, cpuHandle);
            }
            else {
                D3D12_SHADER_RESOURCE_VIEW_DESC view_desc;
                view_desc.Format = DXGI_FORMAT_UNKNOWN;
                view_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                view_desc.Buffer.FirstElement = 0;
                view_desc.Buffer.NumElements = 0;
                view_desc.Buffer.StructureByteStride = 0;
                view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
                //   if (bufferBackend->_init.raw) {
                view_desc.Buffer.StructureByteStride = 0;
                view_desc.Format = DXGI_FORMAT_R32_TYPELESS;
                view_desc.Buffer.Flags |= D3D12_BUFFER_SRV_FLAG_RAW;
                // }*/
                _device->CreateShaderResourceView(nullptr, &view_desc, cpuHandle);
            }
            cpuHandle.ptr += dxDescriptorHeap->_descriptor_increment_size;
        } break;

        case DescriptorType::RW_RESOURCE_BUFFER: {
            auto dxBuffer = static_cast<D3D12BufferBackend*> (descriptorObject._buffer.get());

            ID3D12Resource* resource = dxBuffer->_resource.Get();
            D3D12_UNORDERED_ACCESS_VIEW_DESC* view_desc = &(dxBuffer->_rwResourceBufferView);

            /* if (descriptor->buffers[i]->counter_buffer != NULL) { // Not yet suported, uav with a counter
                ID3D12Resource* counter_resource = descriptor->buffers[i]->counter_buffer->dx_resource;
                p_renderer->dx_device->CreateUnorderedAccessView(resource, counter_resource, view_desc, handle);
            } else*/ {
                _device->CreateUnorderedAccessView(resource, NULL, view_desc, cpuHandle);
            }
            cpuHandle.ptr += dxDescriptorHeap->_descriptor_increment_size;
        } break;

        case DescriptorType::RESOURCE_TEXTURE: {
            auto dxTex = static_cast<D3D12TextureBackend*> (descriptorObject._texture.get());

            if (dxTex) {
                ID3D12Resource* resource = dxTex->_resource.Get();
                D3D12_SHADER_RESOURCE_VIEW_DESC* view_desc = &(dxTex->_shaderResourceViewDesc);
                _device->CreateShaderResourceView(resource, view_desc, cpuHandle);
            }
            else {
                D3D12_SHADER_RESOURCE_VIEW_DESC view_desc;

                auto view_dim = D3D12_SRV_DIMENSION_TEXTURE2D;

                view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                view_desc.ViewDimension = view_dim;
                view_desc.Texture2D.MipLevels = 1;
                view_desc.Texture2D.PlaneSlice = 0;
                view_desc.Texture2D.MostDetailedMip = 0;
                view_desc.Texture2D.ResourceMinLODClamp = 0.0f;
                // }*/
                _device->CreateShaderResourceView(nullptr, &view_desc, cpuHandle);
            }
            cpuHandle.ptr += dxDescriptorHeap->_descriptor_increment_size;
        } break;

        case DescriptorType::RW_RESOURCE_TEXTURE: {
            auto dxTex = static_cast<D3D12TextureBackend*> (descriptorObject._texture.get());

            if (dxTex) {
                ID3D12Resource* resource = dxTex->_resource.Get();
                D3D12_UNORDERED_ACCESS_VIEW_DESC* view_desc = &(dxTex->_unorderedAccessViewDesc);
                _device->CreateUnorderedAccessView(resource, NULL, view_desc, cpuHandle);
            }
            else {
                // this is an empty descriptor slot, probably bad
            }
            cpuHandle.ptr += dxDescriptorHeap->_descriptor_increment_size;
        } break;

        }
    }

    //assuming everything went well, update the objects stored in the descriptorSet

    descriptorSet->_objects = objects;
}


D3D12DescriptorHeapBackend::D3D12DescriptorHeapBackend() {
}

D3D12DescriptorHeapBackend::~D3D12DescriptorHeapBackend() {
}

int32_t D3D12DescriptorHeapBackend::allocateDescriptors(int32_t numDescriptors) {

    return (int32_t) _descriptor_table.allocateContiguous(numDescriptors);
}

int32_t D3D12DescriptorHeapBackend::allocateSamplers(int32_t numSamplers) {
    return  (int32_t) _sampler_table.allocateContiguous(numSamplers);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeapBackend::gpuHandle(uint32_t offset) const {
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = _cbvsrvuav_heap->GetGPUDescriptorHandleForHeapStart();
    gpuHandle.ptr += offset * _descriptor_increment_size;
    return gpuHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeapBackend::cpuHandle(uint32_t offset) const {
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _cbvsrvuav_heap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += offset * _descriptor_increment_size;
    return cpuHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeapBackend::gpuSamplerHandle(uint32_t offset) const {
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = _sampler_heap->GetGPUDescriptorHandleForHeapStart();
    gpuHandle.ptr += offset * _sampler_increment_size;
    return gpuHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeapBackend::cpuSamplerHandle(uint32_t offset) const {
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _sampler_heap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += offset * _sampler_increment_size;
    return cpuHandle;
}
DescriptorHeapPointer D3D12Backend::getDescriptorHeap() {
    return _descriptorHeap;
}

 DescriptorHeapPointer D3D12Backend::createDescriptorHeap(const DescriptorHeapInit& init) {
    auto heap = new D3D12DescriptorHeapBackend();
    heap->_init = init;

    int32_t cbvsrvuav_count = init._numDescritors;
    int32_t sampler_count = init._numSamplers;

    if (cbvsrvuav_count > 0) {
        D3D12_DESCRIPTOR_HEAP_DESC desc;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = cbvsrvuav_count;
        desc.NodeMask = 0;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        HRESULT hres = _device->CreateDescriptorHeap(&desc,
            __uuidof(heap->_cbvsrvuav_heap), (void**)&((heap->_cbvsrvuav_heap)));
    }

    if (sampler_count > 0) {
        D3D12_DESCRIPTOR_HEAP_DESC desc;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        desc.NumDescriptors = sampler_count;
        desc.NodeMask = 0;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        HRESULT hres = _device->CreateDescriptorHeap(&desc,
            __uuidof(heap->_sampler_heap), (void**)&((heap->_sampler_heap)));
    }

    heap->_descriptor_increment_size = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    heap->_sampler_increment_size = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    return DescriptorHeapPointer(heap);
}

#endif
