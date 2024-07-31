// Scene.h 
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

#include "Renderer.h"
#include "render/Transform.h"
#include "render/Draw.h"
#include "render/Item.h"
#include "render/Camera.h"
#include "render/Animation.h"

namespace graphics {
    using UserID  = uint32_t;  

    struct VISUALIZATION_API SceneInit {
        DevicePointer device; // need a device to create the scene

        // And allocate all the capacities for the various stores
        int32_t items_capacity = 100000; 
        int32_t nodes_capacity = 100000;
        int32_t drawables_capacity = 1000;
        int32_t cameras_capacity = 10;
        int32_t animations_capacity = 1000;
    };

    class VISUALIZATION_API Scene {
    public:
       
        Scene(const SceneInit& init);
        ~Scene();

        // Items
        ItemStore _items;
        
        using ItemInit = ItemStore::ItemInit;
        Item createItem(const ItemInit& init, UserID userID = INVALID_ITEM_ID);

/*        Item createItem(Node node, Draw draw, UserID userID = INVALID_ITEM_ID);
        Item createItem(NodeID node, DrawID draw, UserID userID = INVALID_ITEM_ID);
        Item createSubItem(ItemID group, NodeID node, DrawID draw, UserID userID = INVALID_ITEM_ID);
        Item createSubItem(ItemID group, Node node, Draw draw, UserID userID = INVALID_ITEM_ID);
*/
        void deleteAll();
        void deleteItem(ItemID id);
        inline Item getItem(ItemID id) const { return _items.getItem(id); } // Item is either null or valid
        inline Item getValidItemAt(uint32_t startIndex) const { return _items.getValidItemAt(startIndex); } // next valid Item found at startId or after OR return a null item if none valid after
        inline Item getUnsafeItem(ItemID id) const { return _items.getUnsafeItem(id); } // ItemID is not checked for validity and could return an invalid item
        inline ItemIDs fetchValidItems() const { return _items.fetchValidItems(); }

        // Access item from a UserID
        void deleteAllItemsWithUserID();
        void deleteItemFromUserID(UserID id);
        Item getItemFromUserID(UserID id) const;

        // Nodes
        NodeStore _nodes;
        inline Node getNode(NodeID id) const { return _nodes.getNode(id); } // Node is either null or valid

        using NodeInit = NodeStore::NodeInit;
        Node createNode(const NodeInit& init);
        using NodeBranchInit = NodeStore::NodeBranchInit;
        NodeIDs createNodeBranch(const NodeBranchInit& init);
        void deleteNode(NodeID nodeId);

        void attachNode(NodeID child, NodeID parent);
        void detachNode(NodeID child);


        // Draws
        DrawStore _drawables;
        template <typename T>
        Draw createDraw(T x) {
            return _drawables.createDraw(std::move(x));
        }

        // Cameras
        CameraStore _cameras;
        CameraPointer getCamera(CameraID camId) const;
        CameraPointer createCamera() {
            return _cameras.createCamera();
        }

        // Bounds
        void updateBounds();
        const core::Bounds& getBounds() const { return _bounds; }

        
        // Animations
        AnimStore _anims;
        template <typename T>
        Anim createAnim(T x) {
            return _anims.createAnim(std::move(x));
        }

        // TODO: Find a better way....
        SkyDrawFactory_sp _skyFactory;
        Sky_sp _sky;

    protected:

        IDToIndices _userIDToItemIDs;

        core::Bounds _bounds;
    };

    // Standard function to synchronise all the gpu resources required by the scene for the frame being constructed
    void syncSceneResourcesForFrame(const ScenePointer& scene, const BatchPointer& batch);
}