// Draw.h 
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
#pragma once

#include <functional>

#include <core/math/Math3D.h>

#include "Renderer.h"
#include "render/Transform.h"
#include "gpu/StructuredBuffer.h"



namespace graphics {

    using DrawID = core::IndexTable::Index;
    using DrawIDs = core::IndexTable::Indices;
    static const DrawID INVALID_DRAW_ID = core::IndexTable::INVALID_INDEX;

    //using Drawcall = void(const NodeID node, RenderArgs& args);
    using DrawObjectCallback = std::function<void(
        const NodeID node,
        RenderArgs& args)>;

    using DrawBound = core::aabox3;


    // 
    // Draw class
    //
    template <typename T> DrawBound drawable_getBound(const T& x) {
        return x.getBound();
    }
    template <typename T> DrawObjectCallback drawable_getDrawcall(const T& x) {
        return x.getDrawcall();
    }

    struct VISUALIZATION_API Draw {
    public:
        static Draw null;
        Draw() {} // Same as the null Draw

        DrawID id() const { return _self->_id; }
        DrawBound getBound() const { return _self->getBound(); }
        DrawObjectCallback getDrawcall() const { return _self->getDrawcall(); }

    private:
        friend class DrawStore;


        template <typename T>
        Draw(T x) :
            _self(std::make_shared<Model<T>>(x)) {
        }


        struct Concept {
            mutable DrawID _id{ INVALID_DRAW_ID };

            virtual ~Concept() = default;

            virtual DrawBound getBound() const = 0;
            virtual DrawObjectCallback getDrawcall() const = 0;

        };
        using DrawConcepts = std::vector<std::shared_ptr<const Concept>>;
     
        template <typename T> struct Model final : Concept {
 
            T _data; // The data is moved per value in the model

            Model(T x) : _data(std::move(x)) {}

            DrawBound getBound() const override { return drawable_getBound(_data); }
            DrawObjectCallback getDrawcall() const override { return drawable_getDrawcall(_data); }
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

    using Draws = std::vector<Draw>;

    // 
    // Draw store
    //
    class VISUALIZATION_API DrawStore {
    public:
        // Right after allocation, MUST call the reserve function to allocate the memory chuncks
        void reserve(const DevicePointer& device, uint32_t capacity);

        static const int32_t INVALID_REFCOUNT_INFO = -1;

        struct DrawInfo {
            DrawBound _local_box;
            float  _spareA;
            mutable int32_t _refCount{ INVALID_REFCOUNT_INFO };

            inline bool isValid() const { return _refCount != INVALID_REFCOUNT_INFO; }
        };
        using DrawInfoStructBuffer = StructuredBuffer<DrawInfo>;
        using DrawInfos = DrawInfoStructBuffer::Array;

    private:
        DrawID allocate(DrawObjectCallback drawcall, const DrawBound& bound, const Draw& draw);

        core::IndexTable _indexTable;
        mutable DrawInfoStructBuffer _drawInfos;

        mutable DrawIDs _touchedElements;

        using ReadLock = std::pair< const DrawInfo*, std::lock_guard<std::mutex>>;
        using WriteLock = std::pair< DrawInfo*, std::lock_guard<std::mutex>>;

        inline ReadLock read(DrawID id) const {
            return  _drawInfos.read(id);
        }
        inline WriteLock write(DrawID id) const {
            _touchedElements.emplace_back(id); // take not of the write for this element
            return  _drawInfos.write(id);
        }

        struct Handle {
            DrawStore* _store = nullptr;
            DrawID     _id = INVALID_DRAW_ID;

            inline bool isValidHandle() const {
                return (_store != nullptr) && (_id != INVALID_DRAW_ID);
            }
        };

        using Drawcalls = std::vector< DrawObjectCallback >;
        mutable Drawcalls _drawcalls;

        struct DefaultModel : Draw::Concept {
            DefaultModel(DrawStore* store) : Draw::Concept(), _store(store) {}

            DrawStore* _store = nullptr;

            DrawBound getBound() const override { return _store->getDrawBound(_id); }
            DrawObjectCallback getDrawcall() const override { return _store->getDrawcall(_id); }
        };

        Draw::DrawConcepts _drawConcepts;

    public:
        
        template <typename T>
        Draw createDraw(T x) {
            auto draw = Draw(std::move(x));
            draw._self->_id = createDraw(draw.getDrawcall(), draw.getBound(), draw);
            return draw;
        }

        DrawID createDraw(DrawObjectCallback drawcall, const DrawBound& bound, const Draw& draw = Draw::null);
        void free(DrawID id);

        int32_t reference(DrawID id);
        int32_t release(DrawID id);

        inline auto numValidDraws() const { return _indexTable.getNumValidElements(); }
        inline auto numAllocatedDraws() const { return _indexTable.getNumAllocatedElements(); }

        DrawIDs fetchValidDraws() const;
        DrawInfos fetchDrawInfos() const; // Collect all the DrawInfos in an array, there could be INVALID drawInfos

        inline bool isValid(DrawID id) const { return getDrawInfo(id).isValid(); }

        inline DrawInfo getDrawInfo(DrawID id) const {
            auto [i, l] = read(id);
            return *i;
        }

        // Access the drawbounds by reference and avoid any copy of the lambda

        inline const DrawBound& getDrawBound(DrawID id) const { return getDrawInfo(id)._local_box; }

        // Access the drawcalls by reference and avoid any copy of the lambda
        // THis should be called from a critical section during the rendering pass
        inline const DrawObjectCallback& getDrawcall(DrawID id) const { return _drawcalls[id]; }


    public:
        // gpu api
        inline BufferPointer getGPUBuffer() const { return _drawInfos.gpu_buffer(); }
        void syncGPUBuffer(const BatchPointer& batch);
    };

    using DrawInfo = DrawStore::DrawInfo;
    using DrawInfos = DrawStore::DrawInfos;
}

