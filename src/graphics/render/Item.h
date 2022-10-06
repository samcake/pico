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
    using IDToIndices = std::unordered_map<ItemID, uint32_t>;



    class VISUALIZATION_API ItemStore {
    public:
        // Right after allocation, MUST call the reserve function to assign the Scene and allocate the memory chuncks
        void reserve(const Scene* scene, const DevicePointer& device, uint32_t  capacity);


        // Guts of an item, ItemInfo and ItemFlags
        enum ItemFlags : uint32_t {
            IS_INVALID = 0xFFFFFFFF, // force the flags to all 1 is a special value meaning the info is invalid
            IS_VISIBLE = 0x00000001,
            IS_CAMERA = 0x00000002,
        };

        struct ItemInfo {
            NodeID _nodeID{ INVALID_NODE_ID };
            DrawableID _drawableID{ INVALID_DRAWABLE_ID };
            ItemID _groupID{ INVALID_ITEM_ID };
            uint32_t _flags{ IS_INVALID }; // by default the Info is invalid!

            inline void setVisible(bool visible) { if (isVisible() != visible) toggleVisible(); }
            inline bool isVisible() const { return ((_flags & IS_VISIBLE) != 0); }
            inline bool toggleVisible() { _flags ^= IS_VISIBLE; return isVisible(); }

            inline void setCamera(bool camera) { if (isCamera() != camera) toggleCamera(); }
            inline bool isCamera() const { return ((_flags & IS_CAMERA) != 0); }
            inline bool toggleCamera() { _flags ^= IS_CAMERA; return isCamera(); }

            inline bool isDrawable() const { return _drawableID != INVALID_DRAWABLE_ID; }

            inline bool isValid() const { return _flags != 0xFFFFFFFF; }
        };
        using ItemInfos = std::vector<ItemInfo>;

    private:
        using ItemStructBuffer = StructuredBuffer<ItemInfo>;

        using ReadLock = std::pair< const ItemInfo*, std::lock_guard<std::mutex>>;
        using WriteLock = std::pair< ItemInfo*, std::lock_guard<std::mutex>>;

        inline ReadLock read(ItemID id) const {
            return  _itemInfos.read(id);
        }
        inline WriteLock write(ItemID id) const {
            _touchedElements.emplace_back(id); // take not of the write for this element
            return  _itemInfos.write(id);
        }

        struct Handle {
            ItemStore* _store = nullptr;
            ItemID     _id = INVALID_ITEM_ID;

            inline ReadLock read() const {
                return  _store->_itemInfos.read(_id);
            }
            inline WriteLock write() const {
                return  _store->_itemInfos.write(_id);
            }

            inline bool isValidHandle() const {
                return (_store != nullptr) && (_id != INVALID_ITEM_ID);
            }
        };

    public:

        // Item struct is  just an interface on a particular ItemINfo stored in the Item Store at ItemID.
        // Item can be copied by value
        struct Item {
        public:
            static Item null;

            Item() {} // null item
            Item(const Item& src) = default;
            Item& Item::operator= (const Item&) = default;

            inline bool isValid() const {
                if (_self.isValidHandle())
                    return store()->isValid(id());
                return false;
            }
            inline ItemID   id() const { return  _self._id; }
            inline const ItemStore* store() const { return _self._store; }
            inline ItemStore*       store()       { return _self._store; }

            inline void setVisible(bool visible) { store()->setVisible(id(), visible); }
            inline bool isVisible() const { return store()->isVisible(id()); }
            inline bool toggleVisible() { return store()->toggleVisible(id()); }

            inline void setCamera(bool camera) { store()->setCamera(id(), camera); }
            inline bool isCamera() const { return store()->isCamera(id()); }
            inline bool toggleCamera() { return store()->toggleCamera(id()); }

            inline NodeID nodeID() const { return store()->getNodeID(id()); }
            inline DrawableID drawableID() const { return store()->getDrawableID(id()); }
            inline ItemID groupID() const { return store()->getGroupID(id()); }

            inline ItemInfo info() const { return store()->getItemInfo(id()); }


            inline core::aabox3 fetchWorldBound() const { return store()->fetchWorldBound(id()); }

            inline ItemIDs fetchItemGroup(ItemID owner) const { return store()->fetchItemGroup(id()); }

        private:
            Handle _self;

            friend ItemStore;
            Item(Handle& h) : _self(h) {}
        };

        using Items = std::vector<Item>;
        using ItemIDMap = std::unordered_map<ItemID, Items>;

        Item createItem(Node node, Drawable drawable, ItemID owner = INVALID_ITEM_ID);
        Item createItem(NodeID node, DrawableID drawable, ItemID owner = INVALID_ITEM_ID);
        void free(ItemID id);
        void freeAll();

        inline auto numValidItems() const { return _indexTable.getNumValidElements(); }
        inline auto numAllocatedItems() const { return _indexTable.getNumAllocatedElements(); }

        inline Item getItem(ItemID id) const { return (isValid(id) ? Item(Handle{const_cast<ItemStore*>(this), id}) : Item::null); }
        inline Item getUnsafeItem(ItemID id) const { return Item(Handle{const_cast<ItemStore*>(this), id }); } // We do not check that the itemID is valid here, so could get a fake valid item
        Item getValidItemAt(ItemID id) const;
        ItemIDs fetchValidItems() const;

        ItemInfos fetchItemInfos() const; // Collect all the ItemInfos in an array, there could be INVALID itemInfos

        // Item interface replicated
        inline bool isValid(ItemID id) const {
            auto [i, l] = read(id);
            return i->isValid();
        }

        inline void setVisible(ItemID id, bool visible) {
            auto [i, l] = write(id);
            i->setVisible(visible);
        }
        inline bool isVisible(ItemID id) const {
            auto [i, l] = read(id);
            return i->isVisible();
        }
        inline bool toggleVisible(ItemID id) {
            auto [i, l] = write(id);
            return i->toggleVisible();
        }

        inline void setCamera(ItemID id, bool isCamera) {
            auto [i, l] = write(id);
            i->setCamera(isCamera);
        }
        inline bool isCamera(ItemID id) const {
            auto [i, l] = read(id);
            return i->isCamera();
        }
        inline bool toggleCamera(ItemID id) {
            auto [i, l] = write(id);
            return i->toggleCamera();
        }

        inline NodeID getNodeID(ItemID id) const {
            auto [i, l] = read(id);
            return i->_nodeID;
        }

        inline DrawableID getDrawableID(ItemID id) const {
            auto [i, l] = read(id);
            return i->_drawableID;
        }

        inline ItemID getGroupID(ItemID id) const {
            auto [i, l] = read(id);
            return i->_groupID;
        }

        inline ItemInfo getItemInfo(ItemID id) const {
            auto [i, l] = read(id);
            return *i;
        }

        core::aabox3 fetchWorldBound(ItemID id) const;
        
        ItemIDs fetchItemGroup(ItemID ownerID) const;

    private:
        Item allocate(NodeID node, DrawableID drawable, ItemID owner = INVALID_ITEM_ID);

        core::IndexTable _indexTable;
        mutable ItemStructBuffer _itemInfos;

        Items _items;

        mutable std::vector<ItemID> _touchedElements;

        const Scene* _scene = nullptr;
    public:
        inline BufferPointer getGPUBuffer() const { return _itemInfos.gpu_buffer(); }
        void syncGPUBuffer(const BatchPointer& batch);
    };

    using Item = ItemStore::Item;
    using Items = ItemStore::Items;
    using ItemIDMap = ItemStore::ItemIDMap;
    using ItemInfo = ItemStore::ItemInfo;
    using ItemInfos = ItemStore::ItemInfos;

    /*
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
    */
}