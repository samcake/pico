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

using namespace pico;

#ifdef _WINDOWS
#define ThrowIfFailed(result) if (FAILED((result))) picoLog() << "FAILED !!!/n";

D3D12DescriptorSetLayoutBackend::D3D12DescriptorSetLayoutBackend() {

}

D3D12DescriptorSetLayoutBackend::~D3D12DescriptorSetLayoutBackend() {

}

DescriptorSetLayoutPointer D3D12Backend::createDescriptorSetLayout(const DescriptorSetLayoutInit& init) {
    if (init._layouts.empty()) {
        return nullptr;
    }

    auto descriptorLayout = new D3D12DescriptorSetLayoutBackend();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData;
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    HRESULT hres = _device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));
    if (FAILED(hres)) {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    const uint32_t numDescriptors = (uint32_t) init._layouts.size();
    uint32_t cbvsrvuav_count = 0;
    uint32_t sampler_count = 0;
    uint32_t push_count = 0;
    for (uint32_t i = 0; i < numDescriptors; ++i) {
        uint32_t count = init._layouts[i]._count;
        switch (init._layouts[i]._type) {
        case DescriptorType::SAMPLER: sampler_count += count; break;
        case DescriptorType::UNIFORM_BUFFER: cbvsrvuav_count += count; break;
        case DescriptorType::RESOURCE_BUFFER: cbvsrvuav_count += count; break;
        case DescriptorType::STORAGE_BUFFER_UAV: cbvsrvuav_count += count; break;
        case DescriptorType::RESOURCE_TEXTURE: cbvsrvuav_count += count; break;
        case DescriptorType::TEXTURE_UAV: cbvsrvuav_count += count; break;
        case DescriptorType::UNIFORM_TEXEL_BUFFER_SRV: cbvsrvuav_count += count; break;
        case DescriptorType::STORAGE_TEXEL_BUFFER_UAV: cbvsrvuav_count += count; break;
        case DescriptorType::PUSH_UNIFORM: push_count += count; break;
        }
    }


    // Allocate the param indices (one per descriptor)
    descriptorLayout->_init = init;
    descriptorLayout->_dxParamIndices.resize(numDescriptors);
    descriptorLayout->sampler_count = sampler_count;
    descriptorLayout->cbvsrvuav_count = cbvsrvuav_count;

    // Allocate everything with an upper bound of descriptor counts
    uint32_t range_count = 0;
    uint32_t parameter_count = 0;

    auto ranges_11 = std::vector<D3D12_DESCRIPTOR_RANGE1>(numDescriptors);//*)calloc(numDescriptors, sizeof(*ranges_11));
    auto ranges_10 = std::vector<D3D12_DESCRIPTOR_RANGE>(numDescriptors); //*)calloc(numDescriptors, sizeof(*ranges_10));

    auto parameters_11 = std::vector<D3D12_ROOT_PARAMETER1>(numDescriptors);//*)calloc(numDescriptors, sizeof(*parameters_11));
    auto parameters_10 = std::vector<D3D12_ROOT_PARAMETER>(numDescriptors);//*)calloc(numDescriptors, sizeof(*parameters_11));

    // Build ranges
    for (uint32_t descriptor_index = 0; descriptor_index < numDescriptors; ++descriptor_index) {
        const auto& descriptor = init._layouts[descriptor_index];
        D3D12_DESCRIPTOR_RANGE1* range_11 = &ranges_11[range_count];
        D3D12_DESCRIPTOR_RANGE* range_10 = &ranges_10[range_count];
        D3D12_ROOT_PARAMETER1* param_11 = &parameters_11[parameter_count];
        D3D12_ROOT_PARAMETER* param_10 = &parameters_10[parameter_count];
        param_11->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param_10->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        if (descriptor._type == DescriptorType::PUSH_UNIFORM) {
            param_11->ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            param_10->ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        }

        // Start out with visibility on all shader stages
        param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        uint32_t shader_stage_count = 0;
        // Select one if there is only one
        int shaderStage = (int) descriptor._shaderStage;
        if (shaderStage & ((int) ShaderStage::VERTEX)) {
            param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
            param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
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
            param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            ++shader_stage_count;
        }
   /*     if (shaderStage & ((int)ShaderStage::COMPUTE)) {
            // Keep D3D12_SHADER_VISIBILITY_ALL for compute shaders
            ++shader_stage_count;
        }*/

        // Go back to all shader stages if there's more than one stage
        if (shader_stage_count > 1) {
            param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        }

        bool assign_range = false;
        switch (descriptor._type) {
        case DescriptorType::SAMPLER: {
            range_11->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            range_10->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            assign_range = true;
        }
                                        break;
        case DescriptorType::RESOURCE_BUFFER:
        case DescriptorType::UNIFORM_TEXEL_BUFFER_SRV:
        case DescriptorType::RESOURCE_TEXTURE: {
            range_11->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            range_10->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            assign_range = true;
        }
                                            break;
        case DescriptorType::UNIFORM_BUFFER: {
            range_11->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            range_10->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            assign_range = true;
        }
                                                    break;
        case DescriptorType::STORAGE_BUFFER_UAV:
        case DescriptorType::STORAGE_TEXEL_BUFFER_UAV:
        case DescriptorType::TEXTURE_UAV: {
            range_11->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            range_10->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            assign_range = true;
        }
                                            break;
        case DescriptorType::PUSH_UNIFORM: {
           // range_11->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
           // range_10->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            assign_range = false;
            param_11->Constants.RegisterSpace = 0;
            param_11->Constants.Num32BitValues = descriptor._count;
            param_11->Constants.ShaderRegister = descriptor._binding;
            param_10->Constants.RegisterSpace = 0;
            param_10->Constants.Num32BitValues = descriptor._count;
            param_10->Constants.ShaderRegister = descriptor._binding;
        }
                                        break;
        }

        if (assign_range) {
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

        descriptorLayout->_dxParamIndices[descriptor_index] = parameter_count;
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


    hres = _device->CreateRootSignature(0, sig_blob->GetBufferPointer(), sig_blob->GetBufferSize(),
        __uuidof(descriptorLayout->_rootSignature), (void**)&(descriptorLayout->_rootSignature));
    picoAssert(SUCCEEDED(hres));

    if (sig_blob) sig_blob->Release();
    if (error_msgs) error_msgs->Release();

    return DescriptorSetLayoutPointer(descriptorLayout);
}


D3D12DescriptorSetBackend::D3D12DescriptorSetBackend() {

}

D3D12DescriptorSetBackend::~D3D12DescriptorSetBackend() {

}

DescriptorSetPointer D3D12Backend::createDescriptorSet(const DescriptorSetInit& init) {

    auto descriptorSet = new D3D12DescriptorSetBackend();
    descriptorSet->_init = init;

    auto layout = static_cast<const D3D12DescriptorSetLayoutBackend*>(init._layout.get());
    uint32_t numDescriptors = (uint32_t) layout->_init._layouts.size();
    uint32_t cbvsrvuav_count = layout->cbvsrvuav_count;
    uint32_t sampler_count = layout->sampler_count;

    if (cbvsrvuav_count > 0) {
        D3D12_DESCRIPTOR_HEAP_DESC desc;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = cbvsrvuav_count;
        desc.NodeMask = 0;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        HRESULT hres = _device->CreateDescriptorHeap(&desc,
            __uuidof(descriptorSet->_cbvsrvuav_heap), (void**)&((descriptorSet->_cbvsrvuav_heap)));
    }

    if (sampler_count > 0) {
        D3D12_DESCRIPTOR_HEAP_DESC desc;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        desc.NumDescriptors = sampler_count;
        desc.NodeMask = 0;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        HRESULT hres = _device->CreateDescriptorHeap(&desc,
            __uuidof(descriptorSet->_sampler_heap), (void**)&((descriptorSet->_sampler_heap)));
        
    }

    // Assign heap offsets
    uint32_t cbvsrvuav_heap_offset = 0;
    uint32_t sampler_heap_offset = 0;

    // for each descriptor we need an offset even if not used:
    descriptorSet->_dxHeapOffsets.resize(numDescriptors, 0);

    for (uint32_t i = 0; i < numDescriptors; ++i) {
        const auto& desc = layout->_init._layouts[i];
        switch (desc._type) {
            case DescriptorType::SAMPLER: {
                descriptorSet->_dxHeapOffsets[i] = sampler_heap_offset;
                sampler_heap_offset += desc._count;
            }
                                           break;

            case DescriptorType::UNIFORM_BUFFER:
            case DescriptorType::RESOURCE_BUFFER:
            case DescriptorType::STORAGE_BUFFER_UAV:
            case DescriptorType::RESOURCE_TEXTURE:
            case DescriptorType::TEXTURE_UAV:
            case DescriptorType::UNIFORM_TEXEL_BUFFER_SRV:
            case DescriptorType::STORAGE_TEXEL_BUFFER_UAV: {
                descriptorSet->_dxHeapOffsets[i] = cbvsrvuav_heap_offset;
                cbvsrvuav_heap_offset += desc._count;
            }
                                               break;
        }
    }

    descriptorSet->cbvsrvuav_count = cbvsrvuav_count;
    descriptorSet->sampler_count = sampler_count;
    descriptorSet->_dxGPUHandles.resize(numDescriptors);
    descriptorSet->_dxRootParameterIndices.resize(numDescriptors, -1);


    return DescriptorSetPointer(descriptorSet);
}


void D3D12Backend::updateDescriptorSet(DescriptorSetPointer& descriptorSet, DescriptorObjects& objects) {
    auto dxDescriptorSet = static_cast<D3D12DescriptorSetBackend*> (descriptorSet.get());

    // Not really efficient, just write less frequently ;)
/*
    uint32_t write_count = 0;
    for (uint32_t i = 0; i < p_descriptor_set->descriptor_count; ++i) {
        tr_descriptor* descriptor = &(p_descriptor_set->descriptors[i]);
        if ((NULL != descriptor->samplers) || (NULL != descriptor->textures) || (NULL != descriptor->uniform_buffers)) {
            ++write_count;
        }
    }
*/

    
    uint32_t write_count = (uint32_t) objects.size();
   // Bail if there's nothing to write
    if (0 == write_count) {
        return;
    }

    auto dxDescriptorSetLayout = static_cast<D3D12DescriptorSetLayoutBackend*> (dxDescriptorSet->_init._layout.get());
    picoAssert(objects.size() == dxDescriptorSet->_dxHeapOffsets.size());

    uint32_t objectId = 0;
    for (uint32_t i = 0; i < dxDescriptorSet->_dxHeapOffsets.size(); ++i) {
        //tr_descriptor* descriptor = &(p_descriptor_set->descriptors[i]);
        auto& descriptorLayout = dxDescriptorSetLayout->_init._layouts[i];
        if (descriptorLayout._type == DescriptorType::PUSH_UNIFORM) {
            continue;
        }

        auto& descriptorObject = objects[objectId];
        objectId++;

  /*      if ((NULL == descriptor->samplers) && (NULL == descriptor->textures) && (NULL == descriptor->uniform_buffers)) {
            continue;
        }
*/

        switch (descriptorLayout._type) {
        case DescriptorType::SAMPLER : {
            picoAssert(descriptorObject._samplers.size() >= descriptorLayout._count);

            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = dxDescriptorSet->_sampler_heap->GetCPUDescriptorHandleForHeapStart();
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = dxDescriptorSet->_sampler_heap->GetGPUDescriptorHandleForHeapStart();
            UINT handle_inc_size = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

            cpuHandle.ptr += dxDescriptorSet->_dxHeapOffsets[i] * handle_inc_size;
            gpuHandle.ptr += dxDescriptorSet->_dxHeapOffsets[i] * handle_inc_size;

            dxDescriptorSet->_dxGPUHandles[i] = gpuHandle;
            dxDescriptorSet->_dxRootParameterIndices[i] = dxDescriptorSetLayout->_dxParamIndices[i];


            for (uint32_t j = 0; j < descriptorLayout._count; ++j) {
                picoAssert((descriptorObject._samplers[j].get()));
                auto dxSampler = static_cast<D3D12SamplerBackend*> (descriptorObject._samplers[j].get());

                D3D12_SAMPLER_DESC* sampler_desc = &(dxSampler->_samplerDesc);
                _device->CreateSampler(sampler_desc, cpuHandle);
                cpuHandle.ptr += handle_inc_size;
            }
        } break;

        case DescriptorType::UNIFORM_BUFFER: {
            picoAssert(descriptorObject._uniformBuffers.size() >= descriptorLayout._count);

            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = dxDescriptorSet->_cbvsrvuav_heap->GetCPUDescriptorHandleForHeapStart();
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = dxDescriptorSet->_cbvsrvuav_heap->GetGPUDescriptorHandleForHeapStart();
            UINT handle_inc_size = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            cpuHandle.ptr += dxDescriptorSet->_dxHeapOffsets[i] * handle_inc_size;
            gpuHandle.ptr += dxDescriptorSet->_dxHeapOffsets[i] * handle_inc_size;

            dxDescriptorSet->_dxGPUHandles[i] = gpuHandle;
            dxDescriptorSet->_dxRootParameterIndices[i] = dxDescriptorSetLayout->_dxParamIndices[i];

            for (uint32_t j = 0; j < descriptorLayout._count; ++j) {
                picoAssert((descriptorObject._uniformBuffers[j].get()));
                auto dxUbo = static_cast<D3D12BufferBackend*> (descriptorObject._uniformBuffers[j].get());

                ID3D12Resource* resource = dxUbo->_resource.Get();
                D3D12_CONSTANT_BUFFER_VIEW_DESC* view_desc = &(dxUbo->_uniformBufferView);
                _device->CreateConstantBufferView(view_desc, cpuHandle);
                cpuHandle.ptr += handle_inc_size;
            }
        } break;

        case DescriptorType::RESOURCE_BUFFER: {
            picoAssert(descriptorObject._buffers.size() >= descriptorLayout._count);

            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = dxDescriptorSet->_cbvsrvuav_heap->GetCPUDescriptorHandleForHeapStart();
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = dxDescriptorSet->_cbvsrvuav_heap->GetGPUDescriptorHandleForHeapStart();
            UINT handle_inc_size = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            cpuHandle.ptr += dxDescriptorSet->_dxHeapOffsets[i] * handle_inc_size;
            gpuHandle.ptr += dxDescriptorSet->_dxHeapOffsets[i] * handle_inc_size;

            dxDescriptorSet->_dxGPUHandles[i] = gpuHandle;
            dxDescriptorSet->_dxRootParameterIndices[i] = dxDescriptorSetLayout->_dxParamIndices[i];

            for (uint32_t j = 0; j < descriptorLayout._count; ++j) {
                picoAssert((descriptorObject._buffers[j].get()));
                auto dxUbo = static_cast<D3D12BufferBackend*> (descriptorObject._buffers[j].get());

                ID3D12Resource* resource = dxUbo->_resource.Get();
                D3D12_SHADER_RESOURCE_VIEW_DESC* view_desc = &(dxUbo->_resourceBufferView);
                _device->CreateShaderResourceView(resource, view_desc, cpuHandle);
                cpuHandle.ptr += handle_inc_size;
            }
        }
        break;

/*        case tr_descriptor_type_storage_buffer_uav:
        case tr_descriptor_type_storage_texel_buffer_uav: {
            assert(NULL != descriptor->buffers);

            D3D12_CPU_DESCRIPTOR_HANDLE handle = p_descriptor_set->dx_cbvsrvuav_heap->GetCPUDescriptorHandleForHeapStart();
            UINT handle_inc_size = p_renderer->dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
            for (uint32_t i = 0; i < descriptor->count; ++i) {
                assert(NULL != descriptor->buffers[i]);

                ID3D12Resource* resource = descriptor->buffers[i]->dx_resource;
                D3D12_UNORDERED_ACCESS_VIEW_DESC* view_desc = &(descriptor->buffers[i]->dx_uav_view_desc);
                if (descriptor->buffers[i]->counter_buffer != NULL) {
                    ID3D12Resource* counter_resource = descriptor->buffers[i]->counter_buffer->dx_resource;
                    p_renderer->dx_device->CreateUnorderedAccessView(resource, counter_resource, view_desc, handle);
                }
                else {
                    p_renderer->dx_device->CreateUnorderedAccessView(resource, NULL, view_desc, handle);
                }
                handle.ptr += handle_inc_size;
            }
        }
                                                        break;

*/
        case DescriptorType::RESOURCE_TEXTURE: {
            picoAssert(descriptorObject._textures.size() >= descriptorLayout._count);

            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = dxDescriptorSet->_cbvsrvuav_heap->GetCPUDescriptorHandleForHeapStart();
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = dxDescriptorSet->_cbvsrvuav_heap->GetGPUDescriptorHandleForHeapStart();
            UINT handle_inc_size = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            cpuHandle.ptr += dxDescriptorSet->_dxHeapOffsets[i] * handle_inc_size;
            gpuHandle.ptr += dxDescriptorSet->_dxHeapOffsets[i] * handle_inc_size;

            dxDescriptorSet->_dxGPUHandles[i] = gpuHandle;
            dxDescriptorSet->_dxRootParameterIndices[i] = dxDescriptorSetLayout->_dxParamIndices[i];

            for (uint32_t j = 0; j < descriptorLayout._count; ++j) {
                picoAssert((descriptorObject._textures[j].get()));
                auto dxTex = static_cast<D3D12TextureBackend*> (descriptorObject._textures[j].get());

                ID3D12Resource* resource = dxTex->_resource.Get();
                D3D12_SHADER_RESOURCE_VIEW_DESC* view_desc = &(dxTex->_shaderResourceViewDesc);
                _device->CreateShaderResourceView(resource, view_desc, cpuHandle);
                cpuHandle.ptr += handle_inc_size;
            }
        }
                                           break;

  /*      case tr_descriptor_type_texture_uav: {
            assert(NULL != descriptor->textures);

            D3D12_CPU_DESCRIPTOR_HANDLE handle = p_descriptor_set->dx_cbvsrvuav_heap->GetCPUDescriptorHandleForHeapStart();
            UINT handle_inc_size = p_renderer->dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
            for (uint32_t i = 0; i < descriptor->count; ++i) {
                assert(NULL != descriptor->textures[i]);

                ID3D12Resource* resource = descriptor->textures[i]->dx_resource;
                D3D12_UNORDERED_ACCESS_VIEW_DESC* view_desc = &(descriptor->textures[i]->dx_uav_view_desc);
                p_renderer->dx_device->CreateUnorderedAccessView(resource, NULL, view_desc, handle);
                handle.ptr += handle_inc_size;
            }
        }
                                           break;*/
        }
    }


    //assuming everything went well, update the objects stored in the descriptorSet

    descriptorSet->_objects = objects;
}

#endif
