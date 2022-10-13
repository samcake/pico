// Item.cpp
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
#include "Item.h"

#include "gpu/Resource.h"
#include "gpu/Device.h"
#include "Drawable.h"
#include "Scene.h"

#include <algorithm>

namespace graphics {

    Item Item::null;


    void ItemStore::reserve(const Scene* scene, const DevicePointer& device, uint32_t capacity) {
        _scene = scene;
        _itemInfos.reserve(device, capacity);
    }

    ItemID ItemStore::allocate(NodeID node, DrawableID drawable, ItemID group) {
        auto [new_id, recycle] = _indexTable.allocate();

        ItemInfo info = { node, drawable, group, IS_VISIBLE };
        _itemInfos.allocate_element(new_id, &info);
        _touchedElements.push_back(new_id);

        return new_id;
    }

    ItemID ItemStore::createItem(NodeID node, DrawableID drawable, ItemID group) {
        return allocate(node, drawable, group);
    }

    void ItemStore::free(ItemID index) {
        if (_indexTable.isValid(index)) {
            _indexTable.free(index);
            _itemInfos.set_element(index, nullptr);
            _touchedElements.push_back(index);
        }
    }

    void ItemStore::freeAll() {

    }

    Item ItemStore::getValidItemAt(ItemID startID) const {
        auto itemCount = numAllocatedItems();
        if (startID < itemCount && startID != INVALID_ITEM_ID) {
            // lock the item store info array as a whole
            // so we can use the unsafe data accessor in the search loop
            auto [begin_info, l] = _itemInfos.read(0);

            do {
                const auto* info = begin_info + startID;
                if (info->isValid()) {
                    return getUnsafeItem(startID);
                }
                startID++;
            } while (startID < itemCount);
        }
        return Item::null;
    }

    ItemIDs ItemStore::fetchValidItems() const {
        // lock the item store info array as a whole
        // so we can use the unsafe data accessor in the search loop
        auto [begin_info, l] = _itemInfos.read(0);

        // Pre allocate the result with the expected size
        ItemIDs validItems(numValidItems(), INVALID_ITEM_ID);

        // Collect all the valid item ids
        auto itemCount = numAllocatedItems();
        for (ItemID i = 0; i < itemCount; ++i) {
            if ((begin_info + i)->isValid())
                validItems.emplace_back(i);
        }

        // done
        return validItems;
    }

    ItemInfos ItemStore::fetchItemInfos() const {
        // lock the item store info array as a whole
        // so we can use the unsafe data accessor in the search loop
        auto [begin_info, l] = _itemInfos.read(0);

        auto itemCount = numAllocatedItems();

        // Pre allocate the result with the expected size
        ItemInfos itemInfos(itemCount, ItemInfo());

        // copy
        memcpy(itemInfos.data(), begin_info, itemCount * sizeof(ItemInfo));

        // done
        return itemInfos;
    }

    core::aabox3 ItemStore::fetchWorldBound(ItemID id) const {
        auto [info, l] = _itemInfos.read(id);
        if ((info->_nodeID != INVALID_NODE_ID) && (info->_drawableID != INVALID_DRAWABLE_ID)) {
            return core::aabox_transformFrom(_scene->_nodes.getNodeTransform(info->_nodeID).world, _scene->_drawables.getBound(info->_drawableID));
        } else {
            return core::aabox3();
        }
    }

    ItemIDs ItemStore::fetchItemGroup(ItemID groupID) const {
        ItemIDs itemGroup;
        if (groupID != INVALID_ITEM_ID) {
            // lock the item store info array as a whole
            // so we can use the unsafe data accessor in the search loop
            auto [info, l] = _itemInfos.read(0);

            auto itemCount = numAllocatedItems();
            for (ItemID i = 0; i < itemCount; ++i) {
                auto info = _itemInfos.unsafe_data(i);
                if (info->isValid() && (info->_groupID == groupID)) {
                    itemGroup.emplace_back(i);
                }
            }
        }
        return itemGroup;
    }

    void ItemStore::syncGPUBuffer(const BatchPointer& batch) {
        // Clean the touched elements
        std::sort(_touchedElements.begin(), _touchedElements.end());
        auto newEnd = std::unique(_touchedElements.begin(), _touchedElements.end());
        if (newEnd != _touchedElements.end()) _touchedElements.erase(newEnd);

        // Sync the gpu buffer version
        _itemInfos.sync_gpu_from_cpu(batch, _touchedElements);

        // Start fresh
        _touchedElements.clear();
    }

}
