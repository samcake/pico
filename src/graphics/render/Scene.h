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
#include "render/Drawable.h"
#include "render/Item.h"
#include "render/Camera.h"

namespace graphics {
    using UserID  = uint32_t;  

    struct VISUALIZATION_API SceneInit {
        DevicePointer device; // need a device to create the scene

        // And allocate all the capacities for the various stores
        int32_t items_capacity = 1000; 
        int32_t nodes_capacity = 1000;
        int32_t drawables_capacity = 100;
        int32_t cameras_capacity = 10;
    };

    class VISUALIZATION_API Scene {
    public:
       
        Scene(const SceneInit& init);
        ~Scene();

        // Items
        ItemStore _items;
        
        Item createItem(Node node, Drawable drawable, UserID userID = INVALID_ITEM_ID);
        Item createItem(NodeID node, DrawableID drawable, UserID userID = INVALID_ITEM_ID);
        Item createSubItem(ItemID group, NodeID node, DrawableID drawable, UserID userID = INVALID_ITEM_ID);
        Item createSubItem(ItemID group, Node node, Drawable drawable, UserID userID = INVALID_ITEM_ID);

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
        Node getNode(NodeID nodeId) const;

        Node createNode(const core::mat4x3& rts, NodeID parent);
        NodeIDs createNodeBranch(NodeID rootParent, const std::vector<core::mat4x3>& rts, const NodeIDs& parentsOffsets);
        void deleteNode(NodeID nodeId);

        void attachNode(NodeID child, NodeID parent);
        void detachNode(NodeID child);


        // Drawables
        DrawableStore _drawables;
        Drawable getDrawable(DrawableID drawableId) const;
        template <typename T>
        Drawable createDrawable(T& x) {
            return _drawables.createDrawable(x);
        }

        // Cameras
        CameraStore _cameras;
        CameraPointer getCamera(CameraID camId) const;
        CameraPointer createCamera() {
            return _cameras.createCamera();
        }

        // Bound;
        void updateBounds();
        const core::Bounds& getBounds() const { return _bounds; }

        
        // TODO: Find a better way....
        SkyDrawableFactory_sp _skyFactory;
        Sky_sp _sky;

    protected:

        IDToIndices _userIDToItemIDs;

        core::Bounds _bounds;
    };

    // Standard function to synchronise all the gpu resources required by the scene for the frame being constructed
    void syncSceneResourcesForFrame(const ScenePointer& scene, const BatchPointer& batch);
}