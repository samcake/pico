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

#include "Sky.h"
#include "drawables/SkyDrawable.h"

namespace graphics {

Scene::Scene(const SceneInit& init) {

    _items.reserve(this, init.device, init.items_capacity);
    _nodes.resizeBuffers(init.device, init.nodes_capacity);
    _drawables.resizeBuffers(init.device, init.drawables_capacity);
    _cameras.reserve(init.device, init.cameras_capacity);


    // A sky drawable factory
    // Assign the sky to the scene here ....
    // TODO: NO!
    _skyFactory = std::make_shared<graphics::SkyDrawableFactory>();
    _skyFactory->allocateGPUShared(init.device);
    _sky = _skyFactory->getUniforms()._sky;
}

Scene::~Scene() {
    deleteAll();
}


Item Scene::createItem(Node node, Drawable drawable, UserID userID) {
    return createItem(node.id(), drawable.id(), userID);
}

Item Scene::createItem(NodeID node, DrawableID drawable, UserID userID) {

    Item newItem = _items.createItem(node, drawable);
    
    _nodes.reference(node);
    _drawables.reference(drawable);

    if (userID != INVALID_ITEM_ID) {
        _userIDToItemIDs[userID] = newItem.id();
    }

    return newItem;
}

Item Scene::createSubItem(ItemID group, NodeID node, DrawableID drawable, UserID userID) {
    Item newItem = _items.createItem(node, drawable, group);

    _nodes.reference(node);
    _drawables.reference(drawable);

    if (userID != INVALID_ITEM_ID) {
        _userIDToItemIDs[userID] = newItem.id();
    }

    return newItem;
}

Item Scene::createSubItem(ItemID group, Node node, Drawable drawable, UserID userID) {
    return createSubItem(group, node.id(), drawable.id(), userID);
}

void Scene::deleteAll() {
    _userIDToItemIDs.clear();
    _items.freeAll();
    // TODO : this is doing nothing really ?
}

void Scene::deleteItem(ItemID id) {
    auto itemInfo = _items.getItemInfo(id);
    if (!itemInfo.isValid())
        return;

    // If item is a group item then also delete group items;
    auto group = _items.fetchItemGroup(id);
    for (auto subItem : group) {
        deleteItem(subItem);
    }

    auto nodeID = itemInfo._nodeID;
    auto drawableID = itemInfo._drawableID;

    if (_nodes.release(nodeID) <= 0) {
        _nodes.deleteNode(nodeID);
    }
    if (_drawables.release(drawableID) <= 0) {
        _drawables.free(drawableID);
    }

    _items.free(id);

}

// Access item from a UserID

// delete all items with a user id
void Scene::deleteAllItemsWithUserID() {
    auto indexIt = _userIDToItemIDs.begin();
    while (_userIDToItemIDs.size() && indexIt != _userIDToItemIDs.end()) {
        deleteItem(indexIt->second);
        indexIt = _userIDToItemIDs.begin();
    }
}

// delete item with a user id
void Scene::deleteItemFromUserID(UserID id) {
    auto indexIt = _userIDToItemIDs.find(id);
    if (indexIt != _userIDToItemIDs.end()) {
        auto removedItemId = indexIt->second;
        _userIDToItemIDs.erase(indexIt); // frmove from id to idx table

        _items.free(removedItemId);
    }
}

// get item with a user id
Item Scene::getItemFromUserID(UserID id) const {
    auto indexIt = _userIDToItemIDs.find(id);
    if (indexIt != _userIDToItemIDs.end()) {
        return _items.getUnsafeItem(indexIt->second);
    }
    return Item::null;
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
    auto itemInfos = _items.fetchItemInfos();

    core::aabox3 b;
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

CameraPointer Scene::getCamera(CameraID camId) const {
    return _cameras.getCamera(camId);
}


void syncSceneResourcesForFrame(const ScenePointer& scene, const BatchPointer& batch) {
    scene->_items.syncGPUBuffer(batch);
    scene->_nodes.updateTransforms();
    scene->_cameras.syncGPUBuffer(batch);

    scene->_sky->updateGPUData(); // arggg

}

}