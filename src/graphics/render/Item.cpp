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

    void Item::Concept::setVisible(bool visible) {
         _store->editInfo(_id)._isVisible = visible;
    }

    void Item::Concept::setNode(Node node) {
        auto& info = _store->editInfo(_id);
        info._nodeID = node.id();
    }
    void Item::Concept::setDrawable(Drawable drawable) {
         _store->editInfo(_id)._drawableID = drawable.id();
    }

    core::aabox3 Item::Concept::fetchWorldBound() const {
        const auto& info = _store->getInfo(_id);
        if ((info._nodeID != INVALID_NODE_ID) && (info._drawableID != INVALID_DRAWABLE_ID)) {
            return core::aabox_transformFrom(_scene->_nodes._tree._worldTransforms[info._nodeID], _scene->_drawables.getBound(info._drawableID));
        } else {
            return core::aabox3();
        }
    }

    ItemID ItemStore::newID() {
        return _indexTable.allocate();
    }

    Item ItemStore::allocate(const Scene* scene, NodeID node, DrawableID drawable, ItemID owner) {

        ItemID new_id = newID();
        Item item(new Item::Concept(scene, this, new_id));

        bool allocated = new_id == (_indexTable.getNumAllocatedElements() - 1);

        if (allocated) {
            _items.push_back(item);
            _itemInfos.push_back({node, drawable, owner, true});
        }
        else {
            _items[new_id] = (item);
            _itemInfos[new_id] = { node, drawable, owner, true };
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
            _itemInfos[index] = ItemInfo();
            _touchedElements.push_back(index);
        }
    }

    void ItemStore::freeAll() {

    }


    ItemIDs ItemStore::getItemGroup(ItemID group) const {
        ItemIDs itemGroup;
        for (uint32_t i = 0; i < _items.size(); ++i) {
            if (_items[i].isValid() && _itemInfos[i]._groupID == group) {
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

    void ItemStore::resizeBuffers(const DevicePointer& device, uint32_t numElements) {

        if (_num_buffers_elements < numElements) {
            auto capacity = (numElements);

            graphics::BufferInit items_buffer_init{};
            items_buffer_init.usage = graphics::ResourceUsage::RESOURCE_BUFFER;
            items_buffer_init.hostVisible = true;
            items_buffer_init.bufferSize = capacity * sizeof(ItemInfo);
            items_buffer_init.firstElement = 0;
            items_buffer_init.numElements = capacity;
            items_buffer_init.structStride = sizeof(ItemInfo);

            _items_buffer = device->createBuffer(items_buffer_init);

            _num_buffers_elements = capacity;
        }
    }

    void ItemStore::syncBuffer() {
        if (_touchedElements.empty()) {
             return;
        }

        if (_itemInfos.size() < _num_buffers_elements) {
            for(auto index : _touchedElements) {
                reinterpret_cast<ItemInfo*>(_items_buffer->_cpuMappedAddress)[index] = _itemInfos[index];
            }
            _touchedElements.clear();
        }
    }

}
