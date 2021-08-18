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

namespace graphics {

Scene::Scene() {
}

Scene::~Scene() {
    deleteAll();
}

Item Scene::createItem(Node node, Drawable drawable, UserID userID) {
    return createItem(node.id(), drawable.id(), userID);
}

Item Scene::createItem(NodeID node, DrawableID drawable, UserID userID) {

    Item newItem = _items.createItem(this, node, drawable);

    if (userID != INVALID_ITEM_ID) {
        _idToIndices[userID] = newItem.id();
    }

    return newItem;
}

Item Scene::getItem(ItemID id) const {
    return _items.getItem(id);
}

const Items& Scene::getItems() const {
    return _items.getItems();
}

void Scene::deleteAll() {
    _idToIndices.clear();
    _items.freeAll();
}

// delete all user objects
void Scene::deleteAllItems() {
    auto indexIt = _idToIndices.begin();
    while (_idToIndices.size() && indexIt != _idToIndices.end())
        deleteItem(indexIt->first);
}


void Scene::deleteItem(ItemID id) {
    _items.free(id);
}

void Scene::deleteItemFromID(UserID id) {
    auto indexIt = _idToIndices.find(id);
    if (indexIt != _idToIndices.end()) {
         auto removedItemIdx = indexIt->second;
         _idToIndices.erase(indexIt); // frmove from id to idx table
    
        _items.free(removedItemIdx);
    }
}


Item Scene::getItemFromID(UserID id) const {
    auto indexIt = _idToIndices.find(id);
    if (indexIt != _idToIndices.end()) {
         return getItems()[indexIt->second];
    }
    return Item::null;
}

Item Scene::getValidItemAt(uint32_t startIndex) const {
    return _items.getValidItemAt(startIndex);
}

// Nodes
Node Scene::getNode(NodeID nodeId) const {
    return Node(&_nodes, nodeId);
}

Node Scene::createNode(const core::mat4x3& rts, NodeID parent) {
    return Node(&_nodes, _nodes.createNode(rts, parent));
}

NodeIDs Scene::createNodeBranch(NodeID rootParent, const std::vector<core::mat4x3>& rts, const NodeIDs& parentOffsets) {
    return _nodes.createNodeBranch(rootParent, rts, parentOffsets);
}


void Scene::deleteNode(NodeID nodeId) {
    _nodes.deleteNode(nodeId);
}

void Scene::attachNode(NodeID child, NodeID parent) {
    _nodes.attachNode(child, parent);
}

void Scene::detachNode(NodeID child) {
    _nodes.detachNode(child);
}

// Drawables
Drawable Scene::getDrawable(DrawableID drawableId) const {
    return _drawables.getDrawable(drawableId);
}


void Scene::updateBounds() {

    core::aabox3 b;
    const auto& itemInfos = _items._itemInfos;
    const auto& transforms = _nodes._tree._worldTransforms;
    const auto& bounds = _drawables._bounds;
    int i = 0;
    for (const auto& info : itemInfos) {
        if (info._nodeID != INVALID_NODE_ID && info._drawableID != INVALID_DRAWABLE_ID) {
            auto ibw = core::aabox_transformFrom(transforms[info._nodeID], bounds[info._drawableID]._local_box);
            if (i == 0) {
                b = ibw;
            } else {
                b = core::aabox3::fromBound(b, ibw);
            } 
            i++;
        }
    }
    _bounds._midPos = b.center;
    _bounds._minPos = b.minPos();
    _bounds._maxPos = b.maxPos();
}

}