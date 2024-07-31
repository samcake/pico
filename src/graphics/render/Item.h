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
#include <core/math/Math3D.h>

#include "render/Transform.h"

#include "render/Draw.h"
#include "render/Animation.h"

#include "render/Renderer.h"

namespace graphics {

    using ItemID = core::IndexTable::Index;
    using ItemIDs = core::IndexTable::Indices;
    static const ItemID INVALID_ITEM_ID = core::IndexTable::INVALID_INDEX;
    using IDToIndices = std::unordered_map<ItemID, uint32_t>;

    using ItemName = std::string;
    using ItemNames = std::vector<ItemName>;

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
           // DrawID _drawID{INVALID_DRAW_ID};
           // AnimID _animID{ INVALID_ANIM_ID };
            uint16_t _drawID{(uint16_t) - 1};
            uint16_t _animID{ (uint16_t) -1 };
            ItemID _groupID{ INVALID_ITEM_ID };
            uint32_t _flags{ IS_INVALID }; // by default the Info is invalid!

            inline void setVisible(bool visible) { if (isVisible() != visible) toggleVisible(); }
            inline bool isVisible() const { return ((_flags & IS_VISIBLE) != 0); }
            inline bool toggleVisible() { _flags ^= IS_VISIBLE; return isVisible(); }

            inline void setCamera(bool camera) { if (isCamera() != camera) toggleCamera(); }
            inline bool isCamera() const { return ((_flags & IS_CAMERA) != 0); }
            inline bool toggleCamera() { _flags ^= IS_CAMERA; return isCamera(); }

            inline bool hasNode() const { return _nodeID != INVALID_NODE_ID; }
            inline bool isDraw() const { return _drawID != (uint16_t)-1; } //INVALID_DRAW_ID; }
            inline bool isAnim() const { return _animID != (uint16_t)-1 ;} //INVALID_ANIM_ID; }
            inline bool isGrouped() const { return _groupID != INVALID_ITEM_ID; }

            inline bool isValid() const { return _flags != 0xFFFFFFFF; }
        };
        using ItemStructBuffer = StructuredBuffer<ItemInfo>;
        using ItemInfos = ItemStructBuffer::Array;
        
        struct ItemInit {
            NodeID node = INVALID_NODE_ID;
            DrawID draw = INVALID_DRAW_ID;
            AnimID anim = INVALID_ANIM_ID;
            ItemID group = INVALID_ITEM_ID;
            std::string name;
        };
        ItemID createItem(const ItemInit& init);

    private:
        ItemID allocate(const ItemInit& init);

        core::IndexTable _indexTable;
        mutable ItemStructBuffer _itemInfos;
        mutable ItemIDs _touchedElements;

        ItemNames _itemNames;

        const Scene* _scene = nullptr;

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

            inline bool isValidHandle() const {
                return (_store != nullptr) && (_id != INVALID_ITEM_ID);
            }
        };

    public:

        // Item struct is  just an interface on a particular ItemInfo stored in the Item Store at ItemID.
        // Item can be copied by value
        struct Item {
        public:
            static Item null;

            Item() {} // null item
            Item(const Item& src) = default;
            Item& operator= (const Item&) = default;

            inline bool isValid() const {
                if (_self.isValidHandle())
                    return store()->isValid(id());
                return false;
            }
            inline ItemID   id() const { return  _self._id; }
            inline const ItemStore* store() const { return _self._store; }
            inline ItemStore* store() { return _self._store; }

            inline ItemInfo info() const { return store()->getItemInfo(id()); }
        
            inline void setVisible(bool visible) { store()->setVisible(id(), visible); }
            inline bool isVisible() const { return store()->isVisible(id()); }
            inline bool toggleVisible() { return store()->toggleVisible(id()); }

            inline void setCamera(bool camera) { store()->setCamera(id(), camera); }
            inline bool isCamera() const { return store()->isCamera(id()); }
            inline bool toggleCamera() { return store()->toggleCamera(id()); }

            inline NodeID nodeID() const { return store()->getNodeID(id()); }
            inline DrawID drawID() const { return store()->getDrawID(id()); }
            inline AnimID animID() const { return store()->getAnimID(id()); }
            inline ItemID groupID() const { return store()->getGroupID(id()); }



            inline core::aabox3 fetchWorldBound() const { return store()->fetchWorldBound(id()); }

            inline ItemIDs fetchItemGroup(ItemID owner) const { return store()->fetchItemGroup(id()); }

        private:
            Handle _self;

            friend ItemStore;
            Item(const Handle& h) : _self(h) {}
        };
        inline Item makeItem(ItemID id) { return { Handle{ this, id } }; }

        void free(ItemID id);
        void freeAll();

        inline auto numValidItems() const { return _indexTable.getNumValidElements(); }
        inline auto numAllocatedItems() const { return _indexTable.getNumAllocatedElements(); }

        inline Item getItem(ItemID id) const { return (isValid(id) ? Item(Handle{ const_cast<ItemStore*>(this), id }) : Item::null); }
        inline Item getUnsafeItem(ItemID id) const { return Item(Handle{ const_cast<ItemStore*>(this), id }); } // We do not check that the itemID is valid here, so could get a fake valid item
        Item getValidItemAt(ItemID id) const;

        ItemIDs fetchValidItems() const;
        ItemInfos fetchItemInfos() const; // Collect all the ItemInfos in an array, there could be INVALID itemInfos

        // Item interface replicated
        inline bool isValid(ItemID id) const { return getItemInfo(id).isValid(); }
  
        inline ItemInfo getItemInfo(ItemID id) const {
            auto [i, l] = read(id);
            return *i;
        }
        inline ItemName getItemName(ItemID id) const {
            auto [i, l] = read(id);
            return _itemNames[id];
        }

        inline void setVisible(ItemID id, bool visible) {
            auto [i, l] = write(id);
            i->setVisible(visible);
        }
        inline bool isVisible(ItemID id) const { return getItemInfo(id).isVisible(); }
        inline bool toggleVisible(ItemID id) {
            auto [i, l] = write(id);
            return i->toggleVisible();
        }

        inline void setCamera(ItemID id, bool isCamera) {
            auto [i, l] = write(id);
            i->setCamera(isCamera);
        }
        inline bool isCamera(ItemID id) const { return getItemInfo(id).isCamera(); }
        inline bool toggleCamera(ItemID id) {
            auto [i, l] = write(id);
            return i->toggleCamera();
        }

        inline NodeID getNodeID(ItemID id) const { return getItemInfo(id)._nodeID; }
        inline DrawID getDrawID(ItemID id) const { return getItemInfo(id)._drawID; }
        inline AnimID getAnimID(ItemID id) const { return getItemInfo(id)._animID; }
        inline ItemID getGroupID(ItemID id) const { return getItemInfo(id)._groupID; }

        core::aabox3 fetchWorldBound(ItemID id) const;

        ItemIDs fetchItemGroup(ItemID ownerID) const;

        struct ItemAccessor {
            ItemID id = INVALID_ITEM_ID;
            const ItemInfo& info;
            const ItemName& name;
            int32_t depth = 0;
        };
        using TravereAccessor = std::function<void(const ItemAccessor& item)>;
        void traverse(TravereAccessor accessor) const;
        ItemID traverseItem(const ItemInfo& itemInfo, ItemID itemId, int32_t depth, TravereAccessor accessor) const;

    public:
        // gpu api
        inline BufferPointer getGPUBuffer() const { return _itemInfos.gpu_buffer(); }
        void syncGPUBuffer(const BatchPointer& batch);
    };

    using Item = ItemStore::Item;
    using ItemFlags = ItemStore::ItemFlags;
    using ItemInfo = ItemStore::ItemInfo;
    using ItemInfos = ItemStore::ItemInfos;
    using ItemAccessor = ItemStore::ItemAccessor;

}