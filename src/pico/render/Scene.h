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
#include <unordered_map>
#include "../core/LinearAlgebra.h"

#include "Renderer.h"

namespace pico {
  
    using DrawObjectCallback = std::function<void(  const core::mat4x3 & transform,
                                                    const CameraPointer & camera,
                                                    const SwapchainPointer & swapchain,
                                                    const DevicePointer & device,
                                                    const BatchPointer & batch)>;

    // Here we define the DrawcallObject as the container of the various pico gpu objects we need to render a pointcloud.
    // this will evolve and probably clean up over time and move the genralized concepts in the visualization library
    class VISUALIZATION_API DrawcallObject {
    public:
#pragma warning(push)
#pragma warning(disable: 4251)
        DrawObjectCallback _drawCallback;
        core::Bounds _bounds;
        core::mat4x3 _transform;

#pragma warning(pop)

        DrawcallObject() : _drawCallback(nullptr) {}
        DrawcallObject(DrawObjectCallback callback) : _drawCallback(callback) {}

        void draw(const CameraPointer& camera,
            const SwapchainPointer& swapchain,
            const DevicePointer& device,
            const BatchPointer& batch);
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
        Item() { } // invalid
        friend class Scene;
    public:
        static const ItemID INVALID_ITEM_ID{ 0xFFFFFFFF };
        static Item null;

        template <typename T> Item(uint32_t index, T x):
            _index(index),
            _self(std::make_shared<Model<T>>(std::move(x))) {
        }

        friend void log(const Item& x, std::ostream& out, size_t position) {
            x._self->_log(out, position);
        }

        friend DrawcallObjectPointer getDrawable(const Item& x) {
            return x._self->_getDrawable();
        }

        bool isValid() const { return (_self != nullptr); }

        uint32_t getIndex() const { return _index; }

        bool isVisible() const { return _self->isVisible(); }
        void setVisible(bool visible) { _self->setVisible(visible); }

    private:
        struct Concept{
            virtual ~Concept() = default;
            virtual void _log(std::ostream& out, size_t position) const = 0;
            virtual DrawcallObjectPointer _getDrawable() const = 0;

            bool isVisible() const { return _isVisible; }
            void setVisible(bool visible) const { _isVisible = visible; }
            
            mutable bool _isVisible { true };
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
        uint32_t _index { INVALID_ITEM_ID };
#pragma warning(push)
#pragma warning(disable: 4251)
        std::shared_ptr<const Concept> _self;
#pragma warning(pop)
    };
    using Items = std::vector<Item>;
    using IDToIndices = std::unordered_map<ItemID, uint32_t>;


    class VISUALIZATION_API Scene {
    public:
        Scene();
        ~Scene();

        template <typename T>
        Item createItem(T& x, ItemID userID = Item::INVALID_ITEM_ID) {
            return _createItem(Item((uint32_t)_items.size(), x), userID);
        }

        void deleteAllItems();  // delete all user objects
        void deleteAll();
        Item deleteItem(ItemID id);

        Item getItemFromID(ItemID id) const;
        const Items& getItems() const { return _items; } // THe all items, beware this contains  INVALID items
        const Item& getValidItemAt(uint32_t startIndex) const;

        const core::Bounds& getBounds() const { return _bounds; }
    protected:
#pragma warning(push)
#pragma warning(disable: 4251)
        Items _items;
        IDToIndices _idToIndices;
#pragma warning(pop)

        core::Bounds _bounds;

        Item _createItem(Item& newItem, ItemID userID);
    };

    class Geometry {
    public:
        Geometry(const ScenePointer& scene) {}
        ~Geometry() {}

    };

}