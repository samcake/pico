// Drawable.h 
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


namespace graphics {

    using DrawObjectCallback = std::function<void(
        const NodeID node,
        const CameraPointer& camera,
        const SwapchainPointer& swapchain,
        const DevicePointer& device,
        const BatchPointer& batch)>;

    // Here we define the DrawcallObject as the container of the various pico gpu objects we need to render an item.
    // this will evolve and probably clean up over time and move the genralized concepts in the visualization library

    template <typename T> core::aabox3 drawable_getBound(const T& x) {
        return x.getBound();
    }
    template <typename T> DrawObjectCallback drawable_getDrawcall(const T& x) {
        return x.getDrawcall();
    }

    using DrawableID = core::IndexTable::Index;
    using DrawableIDs = core::IndexTable::Indices;
    static const DrawableID INVALID_DRAWABLE_ID = core::IndexTable::INVALID_INDEX;

    class VISUALIZATION_API Drawable {
      

        friend class DrawableStore;

        template <typename T> Drawable(DrawableID id, T x) :
            _self(std::make_shared<Model<T>>(id, std::move(x))) {
        }


        int32_t reference() { return _self->reference(); }
        int32_t release() { return _self->release(); }

    public:
        Drawable() { } // invalid
        static Drawable null;

        DrawableID id() const { return _self->_id; }
        core::aabox3 getBound() const { return _self->getBound(); }
        DrawObjectCallback getDrawcall() const { return _self->getDrawcall(); }

    private:
        struct Concept {
            Concept(DrawableID index) : _id(index) {}
            virtual ~Concept() = default;
            virtual core::aabox3 getBound() const = 0;
            virtual DrawObjectCallback getDrawcall() const = 0;

            const DrawableID _id{ INVALID_DRAWABLE_ID };
            mutable int32_t _refCount{ 0 };

            int32_t reference() const { return ++ _refCount; }
            int32_t release() const { return -- _refCount; }
        };
        template <typename T> struct Model final : Concept {
            Model(DrawableID index, T x) : Concept(index), _data(std::move(x)) {}

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

    class DrawableStore {
        DrawableID newID();
        Drawable allocate(Drawable drawable);
    public:

        template <typename T>
        Drawable createDrawable(T x) {
            return allocate(Drawable(newID(), x));
        }
        
        void free(DrawableID index);
        int32_t reference(DrawableID index);
        int32_t release(DrawableID index);

        core::aabox3 getBound(DrawableID index) const { return _bounds[index]._local_box; }
        DrawObjectCallback getDrawcall(DrawableID index) const { return _drawables[index].getDrawcall(); }
        Drawable getDrawable(DrawableID index) const { return _drawables[index]; }

        struct VISUALIZATION_API GPUDrawableBound {
            core::aabox3 _local_box;
            float  _spareA;
            float  _spareB;
        };
        std::vector<GPUDrawableBound> _bounds;

    protected:
        core::IndexTable _indexTable;
        std::vector<Drawable> _drawables;

        uint32_t  _num_buffers_elements{ 0 };
    public:
        BufferPointer _drawables_buffer;
        void resizeBuffers(const DevicePointer& device, uint32_t  numElements);

       
    };


}

