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
#include "drawables/SkyDraw.h"

namespace graphics {

    Scene::Scene(const SceneInit& init) {

        _items.reserve(this, init.device, init.items_capacity);
        _nodes.reserve(init.device, init.nodes_capacity);
        _drawables.reserve(init.device, init.drawables_capacity);
        _cameras.reserve(init.device, init.cameras_capacity);


        // A sky draw factory
        // Assign the sky to the scene here ....
        // TODO: NO!
        _skyFactory = std::make_shared<graphics::SkyDrawFactory>(init.device);
        _sky = _skyFactory->getUniforms()._sky;

   
    }

    Scene::~Scene() {
        deleteAll();
    }

    Item Scene::createItem(const ItemInit& init, UserID userID) {
        auto newItem = _items.createItem(init);

        _nodes.reference(init.node);
        _drawables.reference(init.draw);
        _anims.reference(init.anim);

        if (userID != INVALID_ITEM_ID) {
            _userIDToItemIDs[userID] = newItem;
        }

        return _items.makeItem(newItem);
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
        auto DrawID = itemInfo._drawID;

        if (_nodes.release(nodeID) <= 0) {
            _nodes.free(nodeID);
        }
        if (_drawables.release(DrawID) <= 0) {
            _drawables.free(DrawID);
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

    Node Scene::createNode(const NodeInit& init) {
        return _nodes.makeNode(_nodes.createNode(init));
    }

    NodeIDs Scene::createNodeBranch(const NodeBranchInit& init) {
        return _nodes.createNodeBranch(init);
    }


    void Scene::deleteNode(NodeID nodeId) {
        _nodes.free(nodeId);
    }

    void Scene::attachNode(NodeID child, NodeID parent) {
        _nodes.attachNode(child, parent);
    }

    void Scene::detachNode(NodeID child) {
        _nodes.detachNode(child);
    }

    // Draws
 /*   Draw Scene::getDraw(DrawID DrawID) const {
        return _drawables.getDraw(DrawID);
    }*/


    void Scene::updateBounds() {
        auto itemInfos = _items.fetchItemInfos();
        auto nodeTransforms = _nodes.fetchNodeTransforms();
        auto drawInfos = _drawables.fetchDrawInfos();

        core::aabox3 b;
        int i = 0;
        for (const auto& info : itemInfos) {
            if (info._nodeID != INVALID_NODE_ID && info._drawID != INVALID_DRAW_ID) {
                auto ibw = core::aabox_transformFrom(nodeTransforms[info._nodeID].world, drawInfos[info._drawID]._local_box);
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
        scene->_nodes.syncGPUBuffer(batch);
        scene->_drawables.syncGPUBuffer(batch);
        scene->_cameras.syncGPUBuffer(batch);

        scene->_sky->updateGPUData(); // arggg

    }

}