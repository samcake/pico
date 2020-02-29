// Scene.h 
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

#include "../Forward.h"
#include <vector>

#include "Renderer.h"
#include "core/LinearAlgebra.h"

namespace pico {

    // Here we define the DrawcallObject as the container of the various pico gpu objects we need to render a pointcloud.
    // this will evolve and probably clean up over time and move the genralized concepts in the visualization library
    class VISUALIZATION_API DrawcallObject {
    public:
#pragma warning(push)
#pragma warning(disable: 4251)
        RenderCallback _renderCallback;
        core::Bounds _bounds;

#pragma warning(pop)

        DrawcallObject() : _renderCallback(nullptr) {}
        DrawcallObject(RenderCallback callback) : _renderCallback(callback) {}

    };
    using DrawcallObjectPointer = std::shared_ptr<DrawcallObject>;

    template <typename T> void log(const T& x, std::ostream& out, size_t position) {
        out << std::string(position, ' ') << std::endl;
    }

    template <typename T> DrawcallObjectPointer getDrawable(const T& x) {
        return x->getDrawable();
    }

    using ItemID = uint32_t;

    class VISUALIZATION_API Item {
    public:
        static const ItemID INVALID_ITEM_ID{ 0xFFFFFFFF };

        template <typename T> Item(ItemID id, T x):
            _id(id),
            _self(std::make_shared<Model<T>>(std::move(x))) {
        }

        friend void log(const Item& x, std::ostream& out, size_t position) {
            x._self->_log(out, position);
        }

        friend DrawcallObjectPointer getDrawable(const Item& x) {
            return x._self->_getDrawable();
        }

    private:
        struct Concept{
            virtual ~Concept() = default;
            virtual void _log(std::ostream& out, size_t position) const = 0;
            virtual DrawcallObjectPointer _getDrawable() const = 0;
        };
        template <typename T> struct Model final : Concept {
            Model(T x) : _data(std::move(x)) {}
             void _log(std::ostream& out, size_t position) const override {
                log(_data, out, position);
             }
             DrawcallObjectPointer _getDrawable() const override {
                 return getDrawable(_data);
             }
             T _data;
        };
        ItemID _id { INVALID_ITEM_ID };
#pragma warning(push)
#pragma warning(disable: 4251)
        std::shared_ptr<const Concept> _self;
#pragma warning(pop)
    };
    using Items = std::vector<Item>;


    class VISUALIZATION_API Scene {
    public:
        Scene();
        ~Scene();

        template <typename T>
        Item createItem(T& x) {
            return _createItem(Item((ItemID)_items.size(), x));
        }


        const Items& getItems() const { return _items; }

        const core::Bounds& getBounds() const { return _bounds; }
    protected:
#pragma warning(push)
#pragma warning(disable: 4251)
        Items _items;
#pragma warning(pop)

        core::Bounds _bounds;

        Item _createItem(Item& newItem);
    };

    class Geometry {
    public:
        Geometry(const ScenePointer& scene) {}
        ~Geometry() {}

    };

}