// Item.h 
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

#include <vector>
#include <unordered_map>
#include <iostream>
#include <core/math/LinearAlgebra.h>

#include "render/Transform.h"

#include "render/Drawable.h"

#include "render/Renderer.h"

namespace graphics {

    using ItemID = core::IndexTable::Index;
    using ItemIDs = core::IndexTable::Indices;
    static const ItemID INVALID_ITEM_ID = core::IndexTable::INVALID_INDEX;


    class ItemStore;

    class VISUALIZATION_API Item {
    public:
        static Item null;

        Item(const Item& src) = default;
        Item& Item::operator= (const Item&) = default;

        bool isValid() const { return (_self.get() != nullptr); }
        ItemID id() const { return  _self->id(); }

        void setVisible(bool visible) { _self->setVisible(visible); }
        bool isVisible() const { return _self->isVisible(); }

        void setNode(Node node) { _self->setNode(node); }
        NodeID getNode() const { return _self->getNode(); }

        void setDrawable(Drawable drawable) { _self->setDrawable(drawable); }
        DrawableID getDrawable() const { return _self->getDrawable(); }

        core::aabox3 fetchWorldBound() const { return _self->fetchWorldBound(); }

    private:
        struct Concept {
            const Scene* _scene{ nullptr };
            const ItemStore* _store{ nullptr };
            const ItemID _id{ INVALID_ITEM_ID };

            NodeID _nodeID{ INVALID_NODE_ID };
            DrawableID _drawableID{ INVALID_DRAWABLE_ID };
            bool _isVisible{ true };
    
            Concept(const Scene* scene, const ItemStore* store, ItemID index) :
                _scene(scene),
                _store(store),
                _id(index) {}

            ItemID id() const { return  _id; }

            void setVisible(bool visible);
            bool isVisible() const;
            void setNode(Node node);
            NodeID getNode() const;
            void setDrawable(Drawable drawable);
            DrawableID getDrawable() const;

            core::aabox3 fetchWorldBound() const;
        };

        std::shared_ptr<Concept> _self;

        Item() { } // invalid
        friend class ItemStore;
        Item(Concept* self) : _self(self) { }
    };

    using Items = std::vector<Item>;

    class ItemStore {
        ItemID newID();
        Item allocate(const Scene* scene, NodeID node, DrawableID drawable);
    public:

        struct ItemInfo {
            NodeID _nodeID{ INVALID_NODE_ID };
            DrawableID _drawableID{ INVALID_DRAWABLE_ID };
            uint32_t _isVisible{ true };
            uint32_t spareB;
        };

        Item createItem(const Scene* scene, NodeID node = INVALID_NODE_ID, DrawableID drawable = INVALID_DRAWABLE_ID);
        void free(ItemID index);
        void freeAll();

        Item getItem(ItemID index) const { return _items[index]; }
        Item getValidItemAt(ItemID index) const;

        const Items& getItems() const { return _items; };
        const ItemInfo& getInfo(ItemID index) const { return _itemInfos[index]; }

    protected:
        friend class Item;
        friend class Scene;
        core::IndexTable _indexTable;
        Items _items;
        mutable std::vector<ItemInfo> _itemInfos;
        
        ItemInfo& editInfo(ItemID index) const { _touchedElements.push_back(index);  return _itemInfos[index]; }

        uint32_t  _num_buffers_elements{ 0 };
        mutable std::vector<ItemID> _touchedElements;
    public:
        BufferPointer _items_buffer;
        void resizeBuffers(const DevicePointer& device, uint32_t  numElements);
        void syncBuffer();
    };


    inline bool Item::Concept::isVisible() const { return _store->getInfo(_id)._isVisible; }
    inline NodeID Item::Concept::getNode() const { return _store->getInfo(_id)._nodeID; }
    inline DrawableID Item::Concept::getDrawable() const { return _store->getInfo(_id)._drawableID; }


    using IDToIndices = std::unordered_map<ItemID, uint32_t>;

}