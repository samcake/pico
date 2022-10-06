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

namespace graphics {

    Item Item::null;

    core::aabox3 Item::Concept::fetchWorldBound() const {
        const auto& info = _store->getInfo(_id);
        if ((info._nodeID != INVALID_NODE_ID) && (info._drawableID != INVALID_DRAWABLE_ID)) {
            return core::aabox_transformFrom(_scene->_nodes._tree._worldTransforms[info._nodeID], _scene->_drawables.getBound(info._drawableID));
        } else {
            return core::aabox3();
        }
    }

    Item ItemStore::allocate(const Scene* scene, NodeID node, DrawableID drawable, ItemID owner) {
        auto alloc = _indexTable.allocate();
        auto new_id = alloc.index;
        Item item(new Item::Concept(scene, this, new_id));

        ItemInfo info = { node, drawable, owner, true };
        _itemInfos.allocate_element(new_id, &info);

        if (alloc.recycle) {
            _items[new_id] = (item);
        }
        else {
            _items.push_back(item);
        }          

        _touchedElements.push_back(new_id);

        return item;
    }

    Item ItemStore::createItem(const Scene* scene, Node node, Drawable drawable, ItemID owner) {
        return allocate(scene, node.id(), drawable.id(), owner);
    }
    Item ItemStore::createItem(const Scene* scene, NodeID node, DrawableID drawable, ItemID owner) {
        return allocate(scene, node, drawable, owner);
    }

    void ItemStore::free(ItemID index) {
        if (_indexTable.isValid(index)) {
            _indexTable.free(index);
            _items[index] = Item::null;
            {

                _itemInfos._cpu_array[index] = ItemInfo();
            }
            _touchedElements.push_back(index);
        }
    }

    void ItemStore::freeAll() {

    }


    ItemIDs ItemStore::getItemGroup(ItemID group) const {
        auto lockedInfos = _itemInfos.read(0);
        ItemIDs itemGroup;
        for (uint32_t i = 0; i < _items.size(); ++i) {
            if (_items[i].isValid() && lockedInfos.first[i]._groupID == group) {
                itemGroup.emplace_back(i);
            }
        }
        return itemGroup;
    }

    Item ItemStore::getValidItemAt(ItemID startIndex) const {
        if (startIndex < _items.size()) {
            do {
                const auto* item = _items.data() + startIndex;
                if (item->isValid()) {
                    return (*item);
                }
                startIndex++;
            } while (startIndex < _items.size());
        }
        return Item::null;
    }

    void ItemStore::reserve(const DevicePointer& device, uint32_t capacity) {
        _itemInfos.reserve(device, capacity);
    }

    void ItemStore::syncGPUBuffer(const BatchPointer& batch) {
        if (_touchedElements.empty()) {
            return;
        }
        
        for (auto index : _touchedElements) {
            reinterpret_cast<ItemInfo*>(_itemInfos._gpu_buffer->_cpuMappedAddress)[index] = _itemInfos._cpu_array[index];
        }
        _touchedElements.clear();
        
        _itemInfos.sync_gpu_from_cpu(batch);
    }
 
}
