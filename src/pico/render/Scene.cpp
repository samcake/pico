// Scene.cpp
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
#include "Scene.h"

namespace pico {

Item Item::null;

Scene::Scene() {

}

Scene::~Scene() {
    deleteAll();
}

Item Scene::_createItem(Item& newItem, ItemID userID) {
    _items.emplace_back(newItem);
    if (userID != Item::INVALID_ITEM_ID) {
        _idToIndices[userID] = newItem.getIndex();
    }
 
    return _items.back();
}

void Scene::deleteAll() {
    _idToIndices.clear();
    _items.clear();
}

// delete all user objects
void Scene::deleteAllItems() {
    auto indexIt = _idToIndices.begin();
    while (_idToIndices.size() && indexIt != _idToIndices.end())
        deleteItem(indexIt->first);
}

Item Scene::deleteItem(ItemID id) {
    auto indexIt = _idToIndices.find(id);
    if (indexIt != _idToIndices.end()) {
         auto removedItemIdx = indexIt->second;
         _idToIndices.erase(indexIt); // frmove from id to idx table

         auto item = _items[removedItemIdx];
        if (removedItemIdx + 1 == _items.size()) {
            _items.pop_back();
        } else {
            _items[removedItemIdx] = Item();
        }
   
        return item;
    }
    return Item();
}


Item Scene::getItemFromID(ItemID id) const {
    auto indexIt = _idToIndices.find(id);
    if (indexIt != _idToIndices.end()) {
         return getItems()[indexIt->second];
    }
    return Item();
}

const Item& Scene::getValidItemAt(uint32_t startIndex) const {
    if (startIndex < _items.size()) {
        do {
            const auto* item = _items.data() + startIndex;
            if (item->isValid()) {
                return (*item);
            }
            startIndex++;
        }
        while (startIndex < _items.size());
    }
    return Item::null;
}

}


