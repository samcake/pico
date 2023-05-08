// VKBackend_Descriptor.cpp
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

#include <vector>

using namespace graphics;

#ifdef PICO_VULKAN
#define ThrowIfFailed(result) if (FAILED((result))) picoLog("VKBackend_Descriptor FAILED !!!");

VKRootDescriptorLayoutBackend::VKRootDescriptorLayoutBackend() {

}

VKRootDescriptorLayoutBackend::~VKRootDescriptorLayoutBackend() {

}

//VK_SHADER_VISIBILITY EvalShaderVisibility(ShaderStage _shaderStage) {
//    // Start out with visibility on all shader stages
//    VK_SHADER_VISIBILITY ShaderVisibility = VK_SHADER_VISIBILITY_ALL;
//    uint32_t shader_stage_count = 0;
//    // Select one if there is only one
//    int shaderStage = (int)_shaderStage;
//    if (shaderStage & ((int)ShaderStage::VERTEX)) {
//        ShaderVisibility = VK_SHADER_VISIBILITY_VERTEX;
//        ++shader_stage_count;
//    }
//    /*        if (shaderStage & ((int)ShaderStage::HULL)) {
//                param_11->ShaderVisibility = VK_SHADER_VISIBILITY_HULL;
//                param_10->ShaderVisibility = VK_SHADER_VISIBILITY_HULL;
//                ++shader_stage_count;
//            }
//            if (shaderStage & ((int)ShaderStage::DOMAIN)) {
//                param_11->ShaderVisibility = VK_SHADER_VISIBILITY_DOMAIN;
//                param_10->ShaderVisibility = VK_SHADER_VISIBILITY_DOMAIN;
//                ++shader_stage_count;
//            }
//            if (shaderStage & ((int)ShaderStage::GEOMETRY)) {
//                param_11->ShaderVisibility = VK_SHADER_VISIBILITY_GEOMETRY;
//                param_10->ShaderVisibility = VK_SHADER_VISIBILITY_GEOMETRY;
//                ++shader_stage_count;
//            }*/
//    if (shaderStage & ((int)ShaderStage::PIXEL)) {
//        ShaderVisibility = VK_SHADER_VISIBILITY_PIXEL;
//        ++shader_stage_count;
//    }
//    if (shaderStage & ((int)ShaderStage::COMPUTE)) {
//        // Keep VK_SHADER_VISIBILITY_ALL for compute shaders
//        ++shader_stage_count;
//    }
//    if (shaderStage & ((int)ShaderStage::RAYTRACING)) {
//        ShaderVisibility = VK_SHADER_VISIBILITY_ALL; 
//    }
//
//    // Go back to all shader stages if there's more than one stage
//    if (shader_stage_count > 1) {
//        ShaderVisibility = VK_SHADER_VISIBILITY_ALL;
//    }
//
//    return ShaderVisibility;
// }

RootDescriptorLayoutPointer VKBackend::createRootDescriptorLayout(const RootDescriptorLayoutInit& init) {
    auto descriptorLayout = new VKRootDescriptorLayoutBackend();

    //VK_FEATURE_DATA_ROOT_SIGNATURE featureData;
    //featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    //HRESULT hres = _device->CheckFeatureSupport(VK_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));
    //if (FAILED(hres)) {
    //    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    //}

    //uint32_t numSets = 0;
    //uint32_t numSetResources = 0;
    //uint32_t numSetDescriptors = 0;
    //uint32_t numPushs = 0;
    //uint32_t numSamplers = 0;
    //struct DXSetDesc {
    //    uint32_t count = 0;
    //    std::vector<uint32_t> desc_offsets;
    //};

    //std::vector<DXSetDesc> descriptor_setDescs;
    //for (const auto& dsl : init._setLayouts) {
    //    uint32_t numDescs = dsl.size();
    //    DXSetDesc setDesc;
    //    for (uint32_t i = 0; i < numDescs; ++i) {
    //        uint32_t count = dsl[i]._count;
    //        setDesc.count += count;
    //        setDesc.desc_offsets.emplace_back(i);
    //        numSetDescriptors++;
    //    }
    //    numSets++;
    //    numSetResources += setDesc.count;
    //    descriptor_setDescs.emplace_back(std::move(setDesc));
    //}

    //DXSetDesc sampler_setDesc;
    //{
    //    const auto& dsl = init._samplerLayout;
    //    uint32_t numDescs = dsl.size();
    //    for (uint32_t i = 0; i < numDescs; ++i) {
    //        uint32_t count = dsl[i]._count;
    //        sampler_setDesc.count += count;
    //        sampler_setDesc.desc_offsets.emplace_back(i);
    //        numSamplers++;
    //    }
    //}

    //DXSetDesc push_setDesc;
    //{
    //    const auto& dsl = init._pushLayout;
    //    uint32_t numDescs = dsl.size();
    //    for (uint32_t i = 0; i < numDescs; ++i) {
    //        uint32_t count = dsl[i]._count;
    //        push_setDesc.count += count;
    //        push_setDesc.desc_offsets.emplace_back(i);
    //        numPushs++;
    //    }
    //}


    //// Allocate the param indices (one per descriptor)
    //descriptorLayout->_init = init;

    //descriptorLayout->_dxPushParamIndices.resize(numPushs);
    //descriptorLayout->_push_count = push_setDesc.count;

    //descriptorLayout->_dxSetParamIndices.resize(numSets);
    //descriptorLayout->_cbvsrvuav_count = (!numSets ? 0 : numSetResources);
    //descriptorLayout->_cbvsrvuav_counts.resize(numSets);
    //descriptorLayout->_cbvsrvuav_rootIndex = -1;

    //descriptorLayout->_sampler_count = sampler_setDesc.count;
    //descriptorLayout->_sampler_rootIndex = -1;

    //// Allocate everything with an upper bound of descriptor counts
    //uint32_t range_count = 0;
    //uint32_t parameter_count = 0;

    //auto ranges_11 = std::vector<VK_DESCRIPTOR_RANGE1>(numSetDescriptors + numSamplers);
    //auto ranges_10 = std::vector<VK_DESCRIPTOR_RANGE>(numSetDescriptors + numSamplers);

    //auto parameters_11 = std::vector<VK_ROOT_PARAMETER1>(numSets + (numSamplers > 0) + numPushs);
    //auto parameters_10 = std::vector<VK_ROOT_PARAMETER>(numSets + (numSamplers > 0) + numPushs);

    //// Build ranges
    //if (numPushs) {
    //    for (uint32_t descriptor_index = 0; descriptor_index < numPushs; ++descriptor_index) {
    //        const auto& descriptor = init._pushLayout[descriptor_index];
    //        VK_ROOT_PARAMETER1* param_11 = &parameters_11[parameter_count];
    //        VK_ROOT_PARAMETER* param_10 = &parameters_10[parameter_count];
    //        param_11->ParameterType = VK_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    //        param_10->ParameterType = VK_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;

    //        param_10->ShaderVisibility = param_11->ShaderVisibility = EvalShaderVisibility(descriptor._shaderStage);

    //        param_11->Constants.RegisterSpace = 0;
    //        param_11->Constants.Num32BitValues = descriptor._count;
    //        param_11->Constants.ShaderRegister = descriptor._binding;
    //        param_10->Constants.RegisterSpace = 0;
    //        param_10->Constants.Num32BitValues = descriptor._count;
    //        param_10->Constants.ShaderRegister = descriptor._binding;

    //        descriptorLayout->_dxPushParamIndices[descriptor_index] = parameter_count;
    //        ++parameter_count;
    //    }
    //}

    //if (numSets) {
    //    descriptorLayout->_cbvsrvuav_rootIndex = parameter_count; // grab the parameter index for the first set

    //    for (uint32_t set_index = 0; set_index < numSets; ++set_index) {
    //        const auto& setLayout = init._setLayouts[set_index];
    //        // One more root table for this set
    //        VK_ROOT_PARAMETER1* param_11 = &parameters_11[parameter_count];
    //        VK_ROOT_PARAMETER* param_10 = &parameters_10[parameter_count];
    //        param_11->ParameterType = VK_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    //        param_10->ParameterType = VK_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

    //        // add ranges to hold the n descriptors of this set
    //        uint32_t numDescriptors = setLayout.size();
    //        VK_DESCRIPTOR_RANGE1* begin_range_11 = &ranges_11[range_count];
    //        VK_DESCRIPTOR_RANGE* begin_range_10 = &ranges_10[range_count];

    //        uint32_t shaderStageVisibility = 0;

    //        for (uint32_t descriptor_index = 0; descriptor_index < numDescriptors; ++descriptor_index) {
    //            const auto& descriptor = setLayout[descriptor_index];
    //            VK_DESCRIPTOR_RANGE1* range_11 = &ranges_11[range_count];
    //            VK_DESCRIPTOR_RANGE* range_10 = &ranges_10[range_count];

    //            shaderStageVisibility |= (uint32_t)descriptor._shaderStage;

    //            switch (descriptor._type) {
    //            case DescriptorType::RESOURCE_BUFFER:
    //            case DescriptorType::RESOURCE_TEXTURE: {
    //                range_11->RangeType = VK_DESCRIPTOR_RANGE_TYPE_SRV;
    //                range_10->RangeType = VK_DESCRIPTOR_RANGE_TYPE_SRV;
    //            }
    //            break;
    //            case DescriptorType::UNIFORM_BUFFER: {
    //                range_11->RangeType = VK_DESCRIPTOR_RANGE_TYPE_CBV;
    //                range_10->RangeType = VK_DESCRIPTOR_RANGE_TYPE_CBV;
    //            }
    //            break;
    //            case DescriptorType::RW_RESOURCE_BUFFER:
    //            case DescriptorType::RW_RESOURCE_TEXTURE: {
    //                range_11->RangeType = VK_DESCRIPTOR_RANGE_TYPE_UAV;
    //                range_10->RangeType = VK_DESCRIPTOR_RANGE_TYPE_UAV;
    //            }
    //            break;
    //            }

    //            range_11->NumDescriptors = descriptor._count;
    //            range_11->BaseShaderRegister = descriptor._binding;
    //            range_11->RegisterSpace = 0;
    //            range_11->Flags = VK_DESCRIPTOR_RANGE_FLAG_NONE;
    //            range_11->OffsetInDescriptorsFromTableStart = VK_DESCRIPTOR_RANGE_OFFSET_APPEND;

    //            range_10->NumDescriptors = descriptor._count;
    //            range_10->BaseShaderRegister = descriptor._binding;
    //            range_10->RegisterSpace = 0;
    //            range_10->OffsetInDescriptorsFromTableStart = VK_DESCRIPTOR_RANGE_OFFSET_APPEND;

    //            // COunt the number of resources expected total in the set
    //            descriptorLayout->_cbvsrvuav_counts[set_index] += descriptor._count;

    //            ++range_count;
    //        }
    //        param_10->ShaderVisibility = param_11->ShaderVisibility = EvalShaderVisibility((graphics::ShaderStage)shaderStageVisibility);

    //        param_11->DescriptorTable.pDescriptorRanges = begin_range_11;
    //        param_11->DescriptorTable.NumDescriptorRanges = numDescriptors;

    //        param_10->DescriptorTable.pDescriptorRanges = begin_range_10;
    //        param_10->DescriptorTable.NumDescriptorRanges = numDescriptors;

    //        descriptorLayout->_dxSetParamIndices[set_index] = parameter_count;
    //        ++parameter_count;
    //    }
    //}

    //if (numSamplers) {
    //    // One more root table for samplers
    //    VK_ROOT_PARAMETER1* param_11 = &parameters_11[parameter_count];
    //    VK_ROOT_PARAMETER* param_10 = &parameters_10[parameter_count];
    //    param_11->ParameterType = VK_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    //    param_10->ParameterType = VK_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

    //    // add ranges to hold the n samplers
    //    VK_DESCRIPTOR_RANGE1* begin_range_11 = &ranges_11[range_count];
    //    VK_DESCRIPTOR_RANGE* begin_range_10 = &ranges_10[range_count];

    //    uint32_t shaderStageVisibility = 0;

    //    for (uint32_t descriptor_index = 0; descriptor_index < numSamplers; ++descriptor_index) {
    //        const auto& descriptor = init._samplerLayout[descriptor_index];
    //        VK_DESCRIPTOR_RANGE1* range_11 = &ranges_11[range_count];
    //        VK_DESCRIPTOR_RANGE* range_10 = &ranges_10[range_count];

    //        shaderStageVisibility |= (uint32_t) descriptor._shaderStage;

    //        range_11->RangeType = VK_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    //        range_10->RangeType = VK_DESCRIPTOR_RANGE_TYPE_SAMPLER;

    //        range_11->NumDescriptors = descriptor._count;
    //        range_11->BaseShaderRegister = descriptor._binding;
    //        range_11->RegisterSpace = 0;
    //        range_11->Flags = VK_DESCRIPTOR_RANGE_FLAG_NONE;
    //        range_11->OffsetInDescriptorsFromTableStart = VK_DESCRIPTOR_RANGE_OFFSET_APPEND;

    //        range_10->NumDescriptors = descriptor._count;
    //        range_10->BaseShaderRegister = descriptor._binding;
    //        range_10->RegisterSpace = 0;
    //        range_10->OffsetInDescriptorsFromTableStart = VK_DESCRIPTOR_RANGE_OFFSET_APPEND;

    //        ++range_count;
    //    }

    //    param_10->ShaderVisibility = param_11->ShaderVisibility = EvalShaderVisibility((graphics::ShaderStage) shaderStageVisibility);

    //    param_11->DescriptorTable.pDescriptorRanges = begin_range_11;
    //    param_11->DescriptorTable.NumDescriptorRanges = numSamplers;

    //    param_10->DescriptorTable.pDescriptorRanges = begin_range_10;
    //    param_10->DescriptorTable.NumDescriptorRanges = numSamplers;
    //    
    //    descriptorLayout->_sampler_rootIndex = parameter_count;

    //    ++parameter_count;
    //}

    //VK_ROOT_SIGNATURE_FLAGS flags = VK_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    //if (init._pipelineType == PipelineType::RAYTRACING) {
    //    if (init._localSignature)
    //        flags = VK_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    //    else
    //        flags = VK_ROOT_SIGNATURE_FLAG_NONE;
    //}


    //VK_VERSIONED_ROOT_SIGNATURE_DESC desc;
    //if (D3D_ROOT_SIGNATURE_VERSION_1_1 == featureData.HighestVersion) {
    //    desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    //    desc.Desc_1_1.NumParameters = parameter_count;
    //    desc.Desc_1_1.pParameters = parameters_11.data();
    //    desc.Desc_1_1.NumStaticSamplers = 0;
    //    desc.Desc_1_1.pStaticSamplers = NULL;
    //    desc.Desc_1_1.Flags = flags;
    //}
    //else if (D3D_ROOT_SIGNATURE_VERSION_1_0 == featureData.HighestVersion) {
    //    desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
    //    desc.Desc_1_0.NumParameters = parameter_count;
    //    desc.Desc_1_0.pParameters = parameters_10.data();
    //    desc.Desc_1_0.NumStaticSamplers = 0;
    //    desc.Desc_1_0.pStaticSamplers = NULL;
    //    desc.Desc_1_0.Flags = flags;
    //}

    //ID3DBlob* sig_blob = NULL;
    //ID3DBlob* error_msgs = NULL;
    //if (D3D_ROOT_SIGNATURE_VERSION_1_1 == featureData.HighestVersion) {
    //    hres = fnVKSerializeVersionedRootSignature(&desc, &sig_blob, &error_msgs);
    //}
    //else {
    //    hres = VKSerializeRootSignature(&(desc.Desc_1_0), D3D_ROOT_SIGNATURE_VERSION_1_0, &sig_blob, &error_msgs);
    //}
    //picoAssert(SUCCEEDED(hres));
    //if (error_msgs) {
    //    picoLog((char*)error_msgs->GetBufferPointer());
    //}

    //hres = _device->CreateRootSignature(0, sig_blob->GetBufferPointer(), sig_blob->GetBufferSize(),
    //    __uuidof(descriptorLayout->_rootSignature), (void**)&(descriptorLayout->_rootSignature));
    //picoAssert(SUCCEEDED(hres));

    //if (sig_blob) sig_blob->Release();
    //if (error_msgs) error_msgs->Release();

    return RootDescriptorLayoutPointer(descriptorLayout);
}


VKDescriptorSetBackend::VKDescriptorSetBackend() {

}

VKDescriptorSetBackend::~VKDescriptorSetBackend() {

}

DescriptorSetPointer VKBackend::createDescriptorSet(const DescriptorSetInit& init) {

    auto rootLayout = static_cast<const VKRootDescriptorLayoutBackend*>(init._rootLayout.get());
    auto rootSlot = init._bindSetSlot;
    bool fromRootLayout = (rootLayout);
    bool fromSettLayout = (!init._descriptorSetLayout.empty());

    int32_t  cbvsrvuav_root_index = -1;
    uint32_t cbvsrvuav_count = 0;

    int32_t  sampler_root_index = -1;
    uint32_t sampler_count = 0;

    if (fromRootLayout) {
        if (rootSlot >= 0 && rootSlot < rootLayout->_dxSetParamIndices.size()) {
            cbvsrvuav_root_index = rootLayout->_dxSetParamIndices[rootSlot];
            cbvsrvuav_count = rootLayout->_cbvsrvuav_counts[rootSlot];
        }

        if (init._bindSamplers && rootLayout->_sampler_count) {
            sampler_root_index = rootLayout->_sampler_rootIndex;
            sampler_count = rootLayout->_sampler_count;
        }
    } else if (fromSettLayout) {
        if (rootSlot >= 0) {
            cbvsrvuav_root_index = rootSlot;
            for (const auto& desc : init._descriptorSetLayout) {
                cbvsrvuav_count += desc._count;
            }
        }
    }

    if (cbvsrvuav_count == 0 && sampler_root_index == 0) {
        return nullptr;
    }

    auto descriptorSet = new VKDescriptorSetBackend();
    descriptorSet->_init = init;

    //descriptorSet->_cbvsrvuav_rootIndex = cbvsrvuav_root_index;
    //descriptorSet->_sampler_rootIndex = sampler_root_index;
  
    //descriptorSet->_cbvsrvuav_count = cbvsrvuav_count;
    //descriptorSet->_sampler_count = sampler_count;
    //descriptorSet->_numDescriptors = cbvsrvuav_count + sampler_count;

    //// ALLOCATE DESCRIPTORS IN HEAP
    //auto allocatedDescriptors = _descriptorHeap->allocateDescriptors(cbvsrvuav_count);
    //auto allocatedSamplers = _descriptorHeap->allocateSamplers(sampler_count);
    //auto descriptorHeap = static_cast<VKDescriptorHeapBackend*> (_descriptorHeap.get());

    //// Assign heap offsets
    //descriptorSet->_descriptorOffset = allocatedDescriptors;
    //descriptorSet->_samplerOffset = allocatedSamplers;

    //descriptorSet->_cbvsrvuav_GPUHandle = descriptorHeap->gpuHandle(descriptorSet->_descriptorOffset);
    //descriptorSet->_sampler_GPUHandle = descriptorHeap->gpuSamplerHandle(descriptorSet->_samplerOffset);

    return DescriptorSetPointer(descriptorSet);
}


void VKBackend::updateDescriptorSet(DescriptorSetPointer& descriptorSet, DescriptorObjects& objects) {
    auto dxDescriptorSet = static_cast<VKDescriptorSetBackend*> (descriptorSet.get());
   // auto dxDescriptorHeap = static_cast<VKDescriptorHeapBackend*>(_descriptorHeap.get());

   // 
   // uint32_t write_count = (uint32_t) objects.size();
   //// Bail if there's nothing to write
   // if (0 == write_count) {
   //     return;
   // }

   // if (write_count != dxDescriptorSet->_cbvsrvuav_count + dxDescriptorSet->_sampler_count) {
   //     picoLog("Number of objects assigned on descriptor set does NOT match");
   //     return;
   // }

    //VK_CPU_DESCRIPTOR_HANDLE cpuHandle = dxDescriptorHeap->cpuHandle(dxDescriptorSet->_descriptorOffset);
    //VK_CPU_DESCRIPTOR_HANDLE cpuSamplerHandle = dxDescriptorHeap->cpuSamplerHandle(dxDescriptorSet->_samplerOffset);

    //for (uint32_t objectId = 0; objectId < objects.size(); ++objectId) {
    //    auto& descriptorObject = objects[objectId];

    //    switch (descriptorObject._type) {
    //    case DescriptorType::SAMPLER: {
    //        auto dxSampler = static_cast<VKSamplerBackend*> (descriptorObject._sampler.get());

    //        VK_SAMPLER_DESC* sampler_desc = &(dxSampler->_samplerDesc);
    //        _device->CreateSampler(sampler_desc, cpuSamplerHandle);

    //        cpuSamplerHandle.ptr += dxDescriptorHeap->_sampler_increment_size;
    //    } break;

    //    case DescriptorType::UNIFORM_BUFFER: {
    //        if (descriptorObject._buffer) {
    //            auto dxUbo = static_cast<VKBufferBackend*> (descriptorObject._buffer.get());

    //            IVKResource* resource = dxUbo->_resource.Get();
    //            VK_CONSTANT_BUFFER_VIEW_DESC* view_desc = &(dxUbo->_uniformBufferView);
    //            _device->CreateConstantBufferView(view_desc, cpuHandle);
    //        }
    //        cpuHandle.ptr += dxDescriptorHeap->_descriptor_increment_size;
    //    } break;

    //    case DescriptorType::RESOURCE_BUFFER: {
    //        auto dxBuffer = static_cast<VKBufferBackend*> (descriptorObject._buffer.get());
    //        if (dxBuffer) {
    //            IVKResource* resource = dxBuffer->_resource.Get();
    //            VK_SHADER_RESOURCE_VIEW_DESC* view_desc = &(dxBuffer->_resourceBufferView);
    //            if (dxBuffer->_init.usage &= ResourceUsage::ACCELERATION_STRUCTURE) {
    //                _device->CreateShaderResourceView(nullptr, view_desc, cpuHandle);
    //            } else {
    //                _device->CreateShaderResourceView(resource, view_desc, cpuHandle);
    //            }
    //        }
    //        else {
    //            VK_SHADER_RESOURCE_VIEW_DESC view_desc;
    //            view_desc.Format = DXGI_FORMAT_UNKNOWN;
    //            view_desc.ViewDimension = VK_SRV_DIMENSION_BUFFER;
    //            view_desc.Shader4ComponentMapping = VK_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    //            view_desc.Buffer.FirstElement = 0;
    //            view_desc.Buffer.NumElements = 0;
    //            view_desc.Buffer.StructureByteStride = 0;
    //            view_desc.Buffer.Flags = VK_BUFFER_SRV_FLAG_NONE;
    //            //   if (bufferBackend->_init.raw) {
    //            view_desc.Buffer.StructureByteStride = 0;
    //            view_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    //            view_desc.Buffer.Flags |= VK_BUFFER_SRV_FLAG_RAW;
    //            // }*/
    //            _device->CreateShaderResourceView(nullptr, &view_desc, cpuHandle);
    //        }
    //        cpuHandle.ptr += dxDescriptorHeap->_descriptor_increment_size;
    //    } break;

    //    case DescriptorType::RW_RESOURCE_BUFFER: {
    //        auto dxBuffer = static_cast<VKBufferBackend*> (descriptorObject._buffer.get());

    //        IVKResource* resource = dxBuffer->_resource.Get();
    //        VK_UNORDERED_ACCESS_VIEW_DESC* view_desc = &(dxBuffer->_rwResourceBufferView);

    //        /* if (descriptor->buffers[i]->counter_buffer != NULL) { // Not yet suported, uav with a counter
    //            IVKResource* counter_resource = descriptor->buffers[i]->counter_buffer->dx_resource;
    //            p_renderer->dx_device->CreateUnorderedAccessView(resource, counter_resource, view_desc, handle);
    //        } else*/ {
    //            _device->CreateUnorderedAccessView(resource, NULL, view_desc, cpuHandle);
    //        }
    //        cpuHandle.ptr += dxDescriptorHeap->_descriptor_increment_size;
    //    } break;

    //    case DescriptorType::RESOURCE_TEXTURE: {
    //        auto dxTex = static_cast<VKTextureBackend*> (descriptorObject._texture.get());

    //        if (dxTex) {
    //            IVKResource* resource = dxTex->_resource.Get();
    //            VK_SHADER_RESOURCE_VIEW_DESC* view_desc = &(dxTex->_shaderResourceViewDesc);
    //            _device->CreateShaderResourceView(resource, view_desc, cpuHandle);
    //        }
    //        else {
    //            VK_SHADER_RESOURCE_VIEW_DESC view_desc;

    //            auto view_dim = VK_SRV_DIMENSION_TEXTURE2D;

    //            view_desc.Shader4ComponentMapping = VK_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    //            view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //            view_desc.ViewDimension = view_dim;
    //            view_desc.Texture2D.MipLevels = 1;
    //            view_desc.Texture2D.PlaneSlice = 0;
    //            view_desc.Texture2D.MostDetailedMip = 0;
    //            view_desc.Texture2D.ResourceMinLODClamp = 0.0f;
    //            // }*/
    //            _device->CreateShaderResourceView(nullptr, &view_desc, cpuHandle);
    //        }
    //        cpuHandle.ptr += dxDescriptorHeap->_descriptor_increment_size;
    //    } break;

    //    case DescriptorType::RW_RESOURCE_TEXTURE: {
    //        auto dxTex = static_cast<VKTextureBackend*> (descriptorObject._texture.get());

    //        if (dxTex) {
    //            IVKResource* resource = dxTex->_resource.Get();
    //            VK_UNORDERED_ACCESS_VIEW_DESC* view_desc = &(dxTex->_unorderedAccessViewDesc);
    //            _device->CreateUnorderedAccessView(resource, NULL, view_desc, cpuHandle);
    //        }
    //        else {
    //            // this is an empty descriptor slot, probably bad
    //        }
    //        cpuHandle.ptr += dxDescriptorHeap->_descriptor_increment_size;
    //    } break;

    //    }
    //}

    //assuming everything went well, update the objects stored in the descriptorSet

    descriptorSet->_objects = objects;
}


VKDescriptorHeapBackend::VKDescriptorHeapBackend() {
}

VKDescriptorHeapBackend::~VKDescriptorHeapBackend() {
}

int32_t VKDescriptorHeapBackend::allocateDescriptors(int32_t numDescriptors) {
//
//    return (int32_t) _descriptor_table.allocateContiguous(numDescriptors);
    return 0;
}

int32_t VKDescriptorHeapBackend::allocateSamplers(int32_t numSamplers) {
//    return  (int32_t) _sampler_table.allocateContiguous(numSamplers);
    return 0;
}

//VK_GPU_DESCRIPTOR_HANDLE VKDescriptorHeapBackend::gpuHandle(uint32_t offset) const {
//    VK_GPU_DESCRIPTOR_HANDLE gpuHandle = _cbvsrvuav_heap->GetGPUDescriptorHandleForHeapStart();
//    gpuHandle.ptr += offset * _descriptor_increment_size;
//    return gpuHandle;
//}
//
//VK_CPU_DESCRIPTOR_HANDLE VKDescriptorHeapBackend::cpuHandle(uint32_t offset) const {
//    VK_CPU_DESCRIPTOR_HANDLE cpuHandle = _cbvsrvuav_heap->GetCPUDescriptorHandleForHeapStart();
//    cpuHandle.ptr += offset * _descriptor_increment_size;
//    return cpuHandle;
//}
//
//VK_GPU_DESCRIPTOR_HANDLE VKDescriptorHeapBackend::gpuSamplerHandle(uint32_t offset) const {
//    VK_GPU_DESCRIPTOR_HANDLE gpuHandle = _sampler_heap->GetGPUDescriptorHandleForHeapStart();
//    gpuHandle.ptr += offset * _sampler_increment_size;
//    return gpuHandle;
//}
//
//VK_CPU_DESCRIPTOR_HANDLE VKDescriptorHeapBackend::cpuSamplerHandle(uint32_t offset) const {
//    VK_CPU_DESCRIPTOR_HANDLE cpuHandle = _sampler_heap->GetCPUDescriptorHandleForHeapStart();
//    cpuHandle.ptr += offset * _sampler_increment_size;
//    return cpuHandle;
//}
DescriptorHeapPointer VKBackend::getDescriptorHeap() {
    return _descriptorHeap;
}

 DescriptorHeapPointer VKBackend::createDescriptorHeap(const DescriptorHeapInit& init) {
    auto heap = new VKDescriptorHeapBackend();
    //heap->_init = init;

    //int32_t cbvsrvuav_count = init._numDescritors;
    //int32_t sampler_count = init._numSamplers;

    //if (cbvsrvuav_count > 0) {
    //    VK_DESCRIPTOR_HEAP_DESC desc;
    //    desc.Type = VK_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    //    desc.NumDescriptors = cbvsrvuav_count;
    //    desc.NodeMask = 0;
    //    desc.Flags = VK_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    //    HRESULT hres = _device->CreateDescriptorHeap(&desc,
    //        __uuidof(heap->_cbvsrvuav_heap), (void**)&((heap->_cbvsrvuav_heap)));
    //}

    //if (sampler_count > 0) {
    //    VK_DESCRIPTOR_HEAP_DESC desc;
    //    desc.Type = VK_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    //    desc.NumDescriptors = sampler_count;
    //    desc.NodeMask = 0;
    //    desc.Flags = VK_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    //    HRESULT hres = _device->CreateDescriptorHeap(&desc,
    //        __uuidof(heap->_sampler_heap), (void**)&((heap->_sampler_heap)));
    //}

    //heap->_descriptor_increment_size = _device->GetDescriptorHandleIncrementSize(VK_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    //heap->_sampler_increment_size = _device->GetDescriptorHandleIncrementSize(VK_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    return DescriptorHeapPointer(heap);
}


#endif
