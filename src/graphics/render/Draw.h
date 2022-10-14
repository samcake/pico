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

#include <core/math/LinearAlgebra.h>

#include "Renderer.h"
#include "render/Transform.h"
#include "gpu/StructuredBuffer.h"



namespace graphics {

    using DrawID = core::IndexTable::Index;
    using DrawIDs = core::IndexTable::Indices;
    static const DrawID INVALID_DRAW_ID = core::IndexTable::INVALID_INDEX;

    using DrawObjectCallback = std::function<void(
        const NodeID node,
        RenderArgs& args)>;

    // Here we define the DrawcallObject as the container of the various pico gpu objects we need to render an item.
    // this will evolve and probably clean up over time and move the genralized concepts in the visualization library

    template <typename T> core::aabox3 drawable_getBound(const T& x) {
        return x.getBound();
    }
    template <typename T> DrawObjectCallback drawable_getDrawcall(const T& x) {
        return x.getDrawcall();
    }



    class VISUALIZATION_API Draw {
      

        friend class DrawStore;

        template <typename T> Draw(DrawID id, T x) :
            _self(std::make_shared<Model<T>>(id, std::move(x))) {
        }


        int32_t reference() { return _self->reference(); }
        int32_t release() { return _self->release(); }

    public:
        Draw() { } // invalid
        static Draw null;

        DrawID id() const { return _self->_id; }
        core::aabox3 getBound() const { return _self->getBound(); }
        DrawObjectCallback getDrawcall() const { return _self->getDrawcall(); }

    private:
        struct Concept {
            Concept(DrawID index) : _id(index) {}
            virtual ~Concept() = default;
            virtual core::aabox3 getBound() const = 0;
            virtual DrawObjectCallback getDrawcall() const = 0;

            const DrawID _id{ INVALID_DRAW_ID };
            mutable int32_t _refCount{ 0 };

            int32_t reference() const { return ++ _refCount; }
            int32_t release() const { return -- _refCount; }
        };
        template <typename T> struct Model final : Concept {
            Model(DrawID index, T x) : Concept(index), _data(std::move(x)) {}

            core::aabox3 getBound() const override { return drawable_getBound(_data); }
            DrawObjectCallback getDrawcall() const override { return drawable_getDrawcall(_data); }

            T _data;
        };

        std::shared_ptr<const Concept> _self;
    public:
        template <typename T> T& as() const {
            return (const_cast<Model<T>*> (
                        reinterpret_cast<const Model<T>*>(
                            _self.get()
                        )
                    )->_data); }

    };

    class DrawStore {
        DrawID newID();
        Draw allocate(Draw draw);
    public:

        template <typename T>
        Draw createDraw(T x) {
            return allocate(Draw(newID(), x));
        }
        
        void free(DrawID index);
        int32_t reference(DrawID index);
        int32_t release(DrawID index);

        core::aabox3 getBound(DrawID index) const { return _bounds[index]._local_box; }
        DrawObjectCallback getDrawcall(DrawID index) const { return _drawables[index].getDrawcall(); }
        Draw getDraw(DrawID index) const { return _drawables[index]; }

        struct VISUALIZATION_API GPUDrawBound {
            core::aabox3 _local_box;
            float  _spareA;
            float  _spareB;
        };
        std::vector<GPUDrawBound> _bounds;

    protected:
        core::IndexTable _indexTable;
        std::vector<Draw> _drawables;

        uint32_t  _num_buffers_elements{ 0 };
    public:
        BufferPointer _drawables_buffer;
        void resizeBuffers(const DevicePointer& device, uint32_t  numElements);

       
    };


}

