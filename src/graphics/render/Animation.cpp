// Anim.cpp
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
#include "Animation.h"

#include "core/math/Vec.h"
#include "gpu/Resource.h"
#include "gpu/Device.h"

#include "render/scene.h"


#include <algorithm>
using namespace graphics;

Anim Anim::null;

void AnimStore::reserve(const DevicePointer& device, uint32_t capacity) {
    _animInfos.reserve(device, capacity);
}

AnimID AnimStore::allocate(const Anim& anim) {
    auto [new_id, recycle] = _indexTable.allocate();

    //auto bound = Anim.getBound();

    AnimInfo info = { };
    _animInfos.allocate_element(new_id, &info);
    _touchedElements.push_back(new_id);

    if (recycle) {
        _animConcepts[new_id] = anim._self;
    }
    else {
        _animConcepts.emplace_back(anim._self);
    }

    return new_id;
}


AnimID AnimStore::createAnim(const Anim& anim) {
    return allocate(anim);
}

void AnimStore::free(AnimID id) {
    if (_indexTable.isValid(id)) {
        _indexTable.free(id);
        _animInfos.set_element(id, nullptr);
        _touchedElements.push_back(id);
        _animConcepts[id].reset();
    }
}

AnimIDs AnimStore::fetchValidAnims() const {
    // lock the store info array as a whole
    // so we can use the unsafe data accessor in the search loop
    auto [begin_info, l] = _animInfos.read(0);

    // Pre allocate the result with the expected size
    AnimIDs validAnims(numValidAnims(), INVALID_ANIM_ID);

    // Collect all the valid Anim ids
    auto AnimCount = numAllocatedAnims();
    for (AnimID i = 0; i < AnimCount; ++i) {
        if ((begin_info + i)->isValid())
            validAnims.emplace_back(i);
    }

    // done
    return validAnims;
}

AnimInfos AnimStore::fetchAnimInfos() const {
    // lock the item store info array as a whole
    // so we can use the unsafe data accessor in the search loop
    auto [begin_info, l] = _animInfos.read(0);

    auto animCount = numAllocatedAnims();

    // Pre allocate the result with the expected size
    AnimInfos AnimInfos(animCount, AnimInfo());

    // copy
    memcpy(AnimInfos.data(), begin_info, animCount * sizeof(AnimInfo));

    // done
    return AnimInfos;
}

int32_t AnimStore::reference(AnimID id) {
    if (_indexTable.isValid(id)) {
        auto& the_node = *_animInfos.unsafe_data(id);
        the_node._refCount++;
        _touchedElements.push_back(id);
        return the_node._refCount;
    } else {
        return 0;
    }
}
int32_t AnimStore::release(AnimID id) {
    if (_indexTable.isValid(id)) {
        auto& the_node = *_animInfos.unsafe_data(id);
        the_node._refCount--;
        _touchedElements.push_back(id);
        return the_node._refCount;
    } else {
        return 0;
    }
}

void AnimStore::syncGPUBuffer(const BatchPointer& batch) {
    // Clean the touched elements
    std::sort(_touchedElements.begin(), _touchedElements.end());
    auto newEnd = std::unique(_touchedElements.begin(), _touchedElements.end());
    if (newEnd != _touchedElements.end()) _touchedElements.erase(newEnd);

    // Sync the gpu buffer version
    _animInfos.sync_gpu_from_cpu(batch, _touchedElements);

    // Start fresh
    _touchedElements.clear();
}

std::pair<Key::ClipData, Key::ClipArray> Key::createClipsFromGLTF(document::model::Model& model) {
    IDs sampledAccessors(model._accessors.size(), -1);
    IDSet sampledAccessorsSet;
    IDs sampledBufferViews(model._bufferViews.size(), -1);
    IDSet sampledBufferViewsSet;
    IDs sampledBuffers(model._buffers.size(), -1);
    IDSet sampledBuffersSet;

    IDs animationSamplerOffsets(model._animations.size(), -1);
    int32_t nextSamplerOffset = 0;
    for (int a = 0; a < model._animations.size(); ++a) {
        const auto& sourceClip = model._animations[a];
        animationSamplerOffsets[a] = nextSamplerOffset;
        nextSamplerOffset += sourceClip._samplers.size();

        for (auto s = 0; s < sourceClip._samplers.size(); ++s) {
            auto& sourceSampler = sourceClip._samplers[s];
            sampledAccessors[sourceSampler._input] = sourceSampler._input;
            sampledAccessors[sourceSampler._output] = sourceSampler._output;
            sampledAccessorsSet.insert(sourceSampler._input);
            sampledAccessorsSet.insert(sourceSampler._output);

            auto& sourceAccessorIn = model._accessors[sourceSampler._input];
            auto& sourceAccessorOut = model._accessors[sourceSampler._output];
            sampledBufferViews[sourceAccessorIn._bufferView] = sourceAccessorIn._bufferView;
            sampledBufferViews[sourceAccessorOut._bufferView] = sourceAccessorOut._bufferView;
            sampledBufferViewsSet.insert(sourceAccessorIn._bufferView);
            sampledBufferViewsSet.insert(sourceAccessorOut._bufferView);

            auto& sourceBufferViewIn = model._bufferViews[sourceAccessorIn._bufferView];
            auto& sourceBufferViewOut = model._bufferViews[sourceAccessorOut._bufferView];
            sampledBuffers[sourceBufferViewIn._buffer] = sourceBufferViewIn._buffer;
            sampledBuffers[sourceBufferViewOut._buffer] = sourceBufferViewOut._buffer;
            sampledBuffersSet.insert(sourceBufferViewIn._buffer);
            sampledBuffersSet.insert(sourceBufferViewOut._buffer);
        }
    }
    // fill clipData 
    ClipData clipData;
    Buffer& buffer = clipData._buffer;
    AccessorArray& accessors = clipData._accessors;
    accessors.resize(sampledAccessorsSet.size());
    {
        int ai = 0;
        for (auto a : sampledAccessorsSet) {
            sampledAccessors[a] = ai;

            auto& sa = model._accessors[a];
            auto& bv = model._bufferViews[sa._bufferView];
            auto& b = model._buffers[bv._buffer];

            auto& na = accessors[ai];
            na._byteOffset = buffer.size();
            na._elementCount = sa._elementCount;
            na._elementByteSize = document::model::componentTypeSize(sa._componentType) * document::model::elementTypeComponentCount(sa._elementType);
            if (na._elementByteSize == 4) {
                na._type = 0;
            }
            else if (na._elementByteSize == 12) {
                na._type = 1;
            }
            else if (na._elementByteSize == 16) {
                na._type = 2;
            }

            buffer.resize(buffer.size() + na.byteLength());

            for (int i = 0; i < na._elementCount; ++i) {
                if (na._type == 0) {
                    auto v = document::model::FetchBuffer<float>(i, sa, bv, b);
                    auto m = (float*) (buffer.data() + na._byteOffset + i * na._elementByteSize);
                    *m = v;
                    na._minElementValue.x = core::min(na._minElementValue.x, v);
                    na._maxElementValue.x = core::max(na._minElementValue.x, v);
                }
                else if (na._type == 1) {
                    auto v = document::model::FetchBuffer<core::vec3>(i, sa, bv, b);
                    auto m = (core::vec3*)(buffer.data() + na._byteOffset + i * na._elementByteSize);
                    *m = v;
                    na._minElementValue = core::min(na._minElementValue.as_xyz(), v);
                    na._maxElementValue = core::max(na._minElementValue.as_xyz(), v);
                }
                else if (na._type == 2) {
                    auto v = document::model::FetchBuffer<core::vec4>(i, sa, bv, b);
                    auto m = (core::rotor3*)(buffer.data() + na._byteOffset + i * na._elementByteSize);
                    *m = core::rotor3::make_from_quaternion(v);
                    na._minElementValue = core::min(na._minElementValue, v);
                    na._maxElementValue = core::max(na._minElementValue, v);
                }
            }


            ai++;
        }
    }

    SamplerArray& samplers = clipData._samplers;
    samplers.resize(nextSamplerOffset);

    ClipArray clips;
    clips.resize(model._animations.size());

    for (int a = 0; a < model._animations.size(); ++a) {
        const auto& sourceClip = model._animations[a];

        auto& clip = clips[a];
        clip._name = sourceClip._name;

        for (auto s = 0; s < sourceClip._samplers.size(); ++s) {
            auto& sourceSampler = sourceClip._samplers[s];
            Sampler& sampler = samplers[s + animationSamplerOffsets[a]];
            sampler._input = sampledAccessors[sourceSampler._input];
            sampler._output = sampledAccessors[sourceSampler._output];
            sampler._type = sourceSampler._interpolation;
            sampler._beginTime = accessors[sampler._input]._minElementValue.x;
            sampler._endTime = accessors[sampler._input]._maxElementValue.x;

            clip._beginTime = core::min(clip._beginTime, sampler._beginTime);
            clip._endTime = core::max(clip._endTime, sampler._endTime);
        }

        ChannelArray& channels = clip._channels;
        channels.resize(sourceClip._channels.size());
        for (auto c = 0; c < sourceClip._channels.size(); ++c) {
            auto& sourceChannel = sourceClip._channels[c];
            Channel& channel = channels[c];
            channel._samplerId = sourceChannel._sampler + animationSamplerOffsets[a];
            channel._targetId = sourceChannel._target._node;
            channel._path = sourceChannel._target._path;
        }
    }

    return  {clipData, clips};
}

void Key::animateClip(ClipState& state, AnimState& anim, const ClipArray& clips, const ClipData& data) {
    const auto& clip = clips[anim.clip];
    state.channelStates.resize(clip._channels.size());
    for (int i = 0; i < clip._channels.size(); ++i) {
        const auto& source = clip._channels[i];
        auto& result = state.channelStates[i];
        result.targetId = source._targetId;
        result.targetType = source._path;
        if (source._path == Path::TRANSLATION) {
            result.translation = data.sampleVec3(anim.time, source._samplerId);
        } else if (source._path == Path::ROTATION) {
            result.rotation = data.sampleRotor3(anim.time, source._samplerId);
        }
    }
}

void Key::animateTransformBranch(NodeID branchRoot, const ClipState& state, NodeStore& transforms) {

    for (const auto& channelState : state.channelStates) {

        transforms.editNodeTransform(branchRoot + channelState.targetId, [&](core::mat4x3& rts) -> bool {
            if (channelState.targetType == 0) {
                core::translation(rts, channelState.translation);
                //        picoLogf("p= {} {} {}", channelState.translation.x, channelState.translation.y, channelState.translation.z);
                return true;
            }
            else if (channelState.targetType == 1) {
                core::rotation(rts, channelState.rotation);
                //         picoLogf("r= {} {} {} {}", channelState.rotation.a, channelState.rotation.b.xy, channelState.rotation.b.xz, channelState.rotation.b.yz);
                return true;
            }
            else {

            }
            return true;
            });
    }
}

void KeyAnim::animate(NodeID rootNode, AnimateArgs& args) {
    const auto& clip = _key->_clips[_clip];
    
    float currentTime = fmod(args.time, clip._endTime - clip._beginTime) + clip._beginTime;

    Key::AnimState animState = { currentTime, _clip };
    Key::animateClip(_state, animState, _key->_clips, _key->_data);

    auto targetNodeId = rootNode + 1;
    Key::animateTransformBranch(targetNodeId, _state, args.scene->_nodes);
}
