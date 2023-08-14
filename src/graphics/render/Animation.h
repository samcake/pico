// Anim.h 
//
// Sam Gateau - July 2023
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
#pragma once

#include <functional>
#include <unordered_set>
#include <core/math/Math3D.h>
#include <document/Model.h>
#include <core/Log.h>

#include "Renderer.h"
#include "render/Transform.h"
#include "gpu/StructuredBuffer.h"

namespace graphics {
    class Key {
    public:
        using IDs = std::vector<int32_t>;
        using IDSet = std::unordered_set<int32_t>;
        using BufferViewID = int32_t;
        using AccessorID = int32_t;
        using SamplerID = int32_t;
        using ChannelID = int32_t;

        enum Path : uint8_t {
            TRANSLATION = 0,
            ROTATION,
            SCALE,
            WEIGHTS,
            NONE
        };

        using Buffer = std::vector<uint8_t>;

        struct Accessor {
            uint64_t _byteOffset{ 0 };
            uint32_t _elementCount{ 0 };
            uint32_t _elementByteSize{ 0 };
            uint32_t _type{ 0 };
            uint64_t byteLength() const { return _elementCount * _elementByteSize; }
            core::vec4 _minElementValue = std::numeric_limits<float>::infinity();
            core::vec4 _maxElementValue = -std::numeric_limits<float>::infinity();
        };
        using AccessorArray = std::vector<Accessor>;

        struct Sampler {
            AccessorID _input = -1;
            AccessorID _output = -1;
            int32_t _type = 0;
            int32_t _spare = 0;
            float _beginTime = std::numeric_limits<float>::infinity();
            float _endTime = -std::numeric_limits<float>::infinity();
        };
        using SamplerArray = std::vector<Sampler>;

        struct ClipData {
            Buffer _buffer;
            AccessorArray _accessors;
            SamplerArray _samplers;

            template <typename T> const T* fetch(int32_t i, const Accessor& accessor) const {
                return ((const T*)(_buffer.data() + accessor._byteOffset + i * accessor._elementByteSize));
            }
            const float* fetchScalar(int32_t i, AccessorID aid) const { return fetch<float>(i, _accessors[aid]); }
            const core::vec3* fetchVec3(int32_t i, AccessorID aid) const { return fetch<core::vec3>(i, _accessors[aid]); }
            const core::rotor3* fetchRotor(int32_t i, AccessorID aid) const { return fetch<core::rotor3>(i, _accessors[aid]); }

            float sampleInputTrack(float t, const Accessor& in_access, int32_t& i0, int32_t& i1) const {
                // First find the current interval from the input accessor
                int i_idx_0 = 0;
                int i_idx_1 = i_idx_0;
                float i_val_0 = *fetch<float>(0, in_access);
                float i_val_1 = i_val_0;
                if (t >= i_val_0) {
                    for (int i = 1; i < in_access._elementCount; ++i) {
                        float v = *fetch<float>(i, in_access);
                        if (t < v) {
                            i_val_1 = v;
                            i_idx_1 = i;
                            break;
                        }
                        i_val_0 = v;
                        i_val_1 = v;
                        i_idx_0 = i;
                        i_idx_1 = i;
                    }
                }
                float i_param = (i_val_1 - i_val_0);
                if (i_param > 0.0f) {
                    i_param = (t - i_val_0) / i_param;
                }
                else {
                    i_idx_1 = i_idx_0;
                }
                i0 = i_idx_0;
                i1 = i_idx_1;
                return i_param;
            }

            template <typename T> float sampleTrack(float t, SamplerID sid, const T** v0, const T** v1) const {
                const auto& s = _samplers[sid];
                const auto& in_access = _accessors[s._input];
                int32_t i0, i1;
                float p = sampleInputTrack(t, in_access, i0, i1);
                // picoLogf("time = {} {} {}", i0, i1, p);

                const auto& out_access = _accessors[s._output];
                *v0 = fetch<T>(i0, out_access);
                *v1 = fetch<T>(i1, out_access);
                return p;
            }

            core::vec3 sampleVec3(float t, SamplerID sid) const {
                const core::vec3* v0 = nullptr;
                const core::vec3* v1 = nullptr;
                float p = sampleTrack<core::vec3>(t, sid, &v0, &v1);

                // picoLogf("pos = {} {} {} : {} {} {}", v0->x, v0->y, v0->z, v1->x, v1->y, v1->z);
                return core::mix(*v0, *v1, p);
            }

            core::rotor3 sampleRotor3(float t, SamplerID sid) const {
                const core::rotor3* v0 = nullptr;
                const core::rotor3* v1 = nullptr;
                float p = sampleTrack<core::rotor3>(t, sid, &v0, &v1);

                // picoLogf("ori = {} {} {} {} : {} {} {} {}", v0->a, v0->b.xy, v0->b.xz, v0->b.yz, v1->a, v1->b.xy, v1->b.xz, v1->b.yz);
                return core::slerp(*v0, *v1, p);
            }
        };

        struct Channel {
            SamplerID _samplerId = -1;
            NodeID _targetId;
            int32_t _path = Path::NONE;
            int32_t _spare =0;
        };
        using ChannelArray = std::vector<Channel>;

        struct Clip {
            std::string _name;
            float _beginTime = std::numeric_limits<float>::infinity();
            float _endTime = -std::numeric_limits<float>::infinity();
            ChannelArray _channels;
        };
        using ClipArray = std::vector<Clip>;

        static std::pair<ClipData, ClipArray> createClipsFromGLTF(document::model::Model& model);

        struct ChannelState {
            ChannelState() {};
            int32_t targetId;
            int32_t targetType { 0 };
            union {
                core::rotor3 rotation;
                core::vec3   translation;
            };
        };
        struct ClipState {
            std::vector<ChannelState> channelStates;
        };
        struct AnimState {
            float time;
            int32_t clip;
        };

        static void animateClip(ClipState& state, AnimState& anim, const ClipArray& clips, const ClipData& data);

        static void animateTransformBranch(NodeID branchRoot, const ClipState& state, NodeStore& transforms);
    
        ClipData _data;
        ClipArray _clips;
    };
    using KeyPointer = std::shared_ptr<Key>;

    class KeyAnim {
    public:
        KeyPointer      _key;
        int32_t         _clip;
        Key::ClipState  _state;

        void animate(NodeID rootNode, AnimateArgs& args);
    };
}

namespace graphics {

    // Anim describe high level animation "object" which are called once per anim frame
    using AnimID = core::IndexTable::Index;
    using AnimIDs = core::IndexTable::Indices;
    static const AnimID INVALID_ANIM_ID = core::IndexTable::INVALID_INDEX;

    using AnimArgs = AnimateArgs;
    

    // 
    // Anim class
    //

    template <typename T> void Anim_call(const T& x, NodeID node, AnimArgs& args) {
        return const_cast<T&>(x).animate(node, args);
    }

    struct VISUALIZATION_API Anim {
    public:
        static Anim null;
        Anim() {} // Same as the null Anim

        AnimID id() const { return _self->_id; }
        void call(NodeID node, AnimArgs& args) const { return _self->call(node, args); }

    private:
        friend class AnimStore;


        template <typename T>
        Anim(T x) :
            _self(std::make_shared<Model<T>>(x)) {
        }


        struct Concept {
            mutable AnimID _id{ INVALID_ANIM_ID };

            virtual ~Concept() = default;
            virtual void call(NodeID node, AnimArgs& args) const = 0;

        };
        using AnimConcepts = std::vector<std::shared_ptr<const Concept>>;
     
        template <typename T> struct Model final : Concept {
 
            T _data; // The data is moved per value in the model

            Model(T x) : _data(std::move(x)) {}

            void call(NodeID node, AnimArgs& args) const override { return Anim_call(_data, node, args); };
        };

        std::shared_ptr<const Concept> _self;

    public:
        template <typename T> T& as() const {
            return (const_cast<Model<T>*> (
                reinterpret_cast<const Model<T>*>(
                    _self.get()
                    )
                )->_data);
        }

    };

    using Anims = std::vector<Anim>;

    // 
    // Anim store
    //
    class VISUALIZATION_API AnimStore {
    public:
        // Right after allocation, MUST call the reserve function to allocate the memory chuncks
        void reserve(const DevicePointer& device, uint32_t capacity);

        static const int32_t INVALID_REFCOUNT_INFO = -1;

        struct AnimInfo {
            core::vec3  _spareA;
            mutable int32_t _refCount{ INVALID_REFCOUNT_INFO };

            inline bool isValid() const { return _refCount != INVALID_REFCOUNT_INFO; }
        };
        using AnimInfoStructBuffer = StructuredBuffer<AnimInfo>;
        using AnimInfos = AnimInfoStructBuffer::Array;

    private:
        AnimID allocate(const Anim& anim);

        core::IndexTable _indexTable;
        mutable AnimInfoStructBuffer _animInfos;

        mutable AnimIDs _touchedElements;

        using ReadLock = std::pair< const AnimInfo*, std::lock_guard<std::mutex>>;
        using WriteLock = std::pair< AnimInfo*, std::lock_guard<std::mutex>>;

        inline ReadLock read(AnimID id) const {
            return  _animInfos.read(id);
        }
        inline WriteLock write(AnimID id) const {
            _touchedElements.emplace_back(id); // take not of the write for this element
            return  _animInfos.write(id);
        }

        struct Handle {
            AnimStore* _store = nullptr;
            AnimID     _id = INVALID_ANIM_ID;

            inline bool isValidHandle() const {
                return (_store != nullptr) && (_id != INVALID_ANIM_ID);
            }
        };

        struct DefaultModel : Anim::Concept {
            DefaultModel(AnimStore* store) : Anim::Concept(), _store(store) {}

            AnimStore* _store = nullptr;
            void call(AnimArgs& args) {}
        };

        Anim::AnimConcepts _animConcepts;

    public:
        
        template <typename T>
        Anim createAnim(T x) {
            auto anim = Anim(std::move(x));
            anim._self->_id = createAnim(anim);
            return anim;
        }

        AnimID createAnim(const Anim& anim = Anim::null);
        void free(AnimID id);

        int32_t reference(AnimID id);
        int32_t release(AnimID id);

        inline auto numValidAnims() const { return _indexTable.getNumValidElements(); }
        inline auto numAllocatedAnims() const { return _indexTable.getNumAllocatedElements(); }

        AnimIDs fetchValidAnims() const;
        AnimInfos fetchAnimInfos() const; // Collect all the AnimInfos in an array, there could be INVALID AnimInfos

        inline bool isValid(AnimID id) const { return getAnimInfo(id).isValid(); }

        inline AnimInfo getAnimInfo(AnimID id) const {
            auto [i, l] = read(id);
            return *i;
        }

        inline Anim getAnim(AnimID id) const {
            Anim a = {};
            a._self = _animConcepts[id];
            return a;
        }

        inline void animate(AnimID id, NodeID node, AnimArgs& args) {
            _animConcepts[id]->call(node, args);
        }

    public:
        // gpu api
        inline BufferPointer getGPUBuffer() const { return _animInfos.gpu_buffer(); }
        void syncGPUBuffer(const BatchPointer& batch);
    };

    using AnimInfo = AnimStore::AnimInfo;
    using AnimInfos = AnimStore::AnimInfos;
}

