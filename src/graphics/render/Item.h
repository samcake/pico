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

        Item() {} // null item

        Item(const Item& src) = default;
        Item& Item::operator= (const Item&) = default;

        bool isValid() const { return (_self.get() != nullptr); }
        ItemID id() const { return  _self->id(); }

        void setVisible(bool visible) { _self->setVisible(visible); }
        bool isVisible() const { return _self->isVisible(); }
        bool toggleVisible() { return _self->toggleVisible(); }

        void setAsCamera(bool asCamera) { _self->setCamera(asCamera); }
        bool isCamera() const { return _self->isCamera(); }
        bool toggleCamera() { return _self->toggleCamera(); }

        NodeID getNodeID() const { return _self->getNodeID(); }

        DrawableID getDrawableID() const { return _self->getDrawableID(); }

        ItemID getGroupID() const { return _self->getGroupID(); }

        core::aabox3 fetchWorldBound() const { return _self->fetchWorldBound(); }

    private:
        struct Concept {
            const Scene* _scene{ nullptr };
            const ItemStore* _store{ nullptr };
            const ItemID _id{ INVALID_ITEM_ID };
    
            Concept(const Scene* scene, const ItemStore* store, ItemID index) :
                _scene(scene),
                _store(store),
                _id(index) {}

            ItemID id() const { return  _id; }

            void setVisible(bool visible);
            bool isVisible() const;
            bool toggleVisible();

            void setCamera(bool isCam);
            bool isCamera() const;
            bool toggleCamera();

            void setNode(Node node);
            NodeID getNodeID() const;
            void setDrawable(Drawable drawable);
            DrawableID getDrawableID() const;
            ItemID getGroupID() const;

            core::aabox3 fetchWorldBound() const;
        };

        std::shared_ptr<Concept> _self;

        friend class ItemStore;
        Item(Concept* self) : _self(self) { }
    };

    using Items = std::vector<Item>;
    using ItemIDMap = std::unordered_map<ItemID, Items>;

    class ItemStore {
        Item allocate(const Scene* scene, NodeID node, DrawableID drawable, ItemID owner = INVALID_ITEM_ID);
    public:

        void reserve(const DevicePointer& device, uint32_t  capacity);

        enum Flags {
            IS_VISIBLE = 0x00000001,
            IS_CAMERA = 0x00000002,
        };

        struct ItemInfo {
            NodeID _nodeID{ INVALID_NODE_ID };
            DrawableID _drawableID{ INVALID_DRAWABLE_ID };
            ItemID _groupID{ INVALID_ITEM_ID };
            uint32_t _flags{ IS_VISIBLE };

            inline void setVisible(bool visible) { if (isVisible() != visible) toggleVisible(); }
            inline bool isVisible() const { return ((_flags & IS_VISIBLE) != 0); }
            inline bool toggleVisible() { _flags ^= IS_VISIBLE; return isVisible(); }

            inline void setCamera(bool camera) { if (isCamera() != camera) toggleCamera(); }
            inline bool isCamera() const { return ((_flags & IS_CAMERA) != 0); }
            inline bool toggleCamera() { _flags ^= IS_CAMERA; return isCamera(); }
        };

        Item createItem(const Scene* scene, Node node, Drawable drawable, ItemID owner = INVALID_ITEM_ID);
        Item createItem(const Scene* scene, NodeID node, DrawableID drawable, ItemID owner = INVALID_ITEM_ID);
        void free(ItemID index);
        void freeAll();

        Item getItem(ItemID index) const { return _items[index]; }
        Item getValidItemAt(ItemID index) const;

        const Items& getItems() const { return _items; };
        const ItemInfo& getInfo(ItemID index) const { return *_itemInfos.data(index); }

        ItemIDs getItemGroup(ItemID owner) const;
    protected:
        friend class Item;
        friend class Scene;
        core::IndexTable _indexTable;
        Items _items;
        using ItemInfos = StructuredBuffer<ItemInfo>;

        mutable ItemInfos _itemInfos;
        
        ItemInfo& editInfo(ItemID index) const { _touchedElements.push_back(index);  return *_itemInfos.data(index); }

        mutable std::vector<ItemID> _touchedElements;
    public:
        inline BufferPointer getGPUBuffer() const { return _itemInfos._gpu_buffer; }
        void syncGPUBuffer(const BatchPointer& batch);
    };

    inline void Item::Concept::setVisible(bool visible) { _store->editInfo(_id).setVisible(visible); }
    inline bool Item::Concept::isVisible() const { return _store->getInfo(_id).isVisible(); }
    inline bool Item::Concept::toggleVisible() { return _store->editInfo(_id).toggleVisible(); }
    inline void Item::Concept::setCamera(bool isCamera) { _store->editInfo(_id).setCamera(isCamera); }
    inline bool Item::Concept::isCamera() const { return _store->getInfo(_id).isCamera(); }
    inline bool Item::Concept::toggleCamera() { return _store->editInfo(_id).toggleCamera(); }

    inline void Item::Concept::setNode(Node node) { _store->editInfo(_id)._nodeID = node.id(); }
    inline NodeID Item::Concept::getNodeID() const { return _store->getInfo(_id)._nodeID; }
    inline void Item::Concept::setDrawable(Drawable drawable) { _store->editInfo(_id)._drawableID = drawable.id(); }
    inline DrawableID Item::Concept::getDrawableID() const { return _store->getInfo(_id)._drawableID; }
    inline ItemID Item::Concept::getGroupID() const { return _store->getInfo(_id)._groupID; }


    using IDToIndices = std::unordered_map<ItemID, uint32_t>;

}