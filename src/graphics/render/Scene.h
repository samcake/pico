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

namespace graphics {
    using UserID  = uint32_t;  

    class VISUALIZATION_API Scene {
    public:
        Scene();
        ~Scene();

        // Items
        ItemStore _items;
        Item getItem(ItemID id) const;
        
        Item createItem(Node node, Drawable drawable, UserID userID = INVALID_ITEM_ID);
        Item createItem(NodeID node, DrawableID drawable, UserID userID = INVALID_ITEM_ID);
        Item createSubItem(ItemID group, NodeID node, DrawableID drawable, UserID userID = INVALID_ITEM_ID);
        Item createSubItem(ItemID group, Node node, Drawable drawable, UserID userID = INVALID_ITEM_ID);

        void deleteAllItems();  // delete all user objects
        void deleteAll();
        void deleteItem(ItemID id);
        void deleteItemFromID(UserID id);

        Item getItemFromID(UserID id) const;
        Item getValidItemAt(uint32_t startIndex) const;
        const Items& getItems() const; // THe all items, beware this contains  INVALID items

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

        // Bound;
        void updateBounds();
        const core::Bounds& getBounds() const { return _bounds; }

        SkyPointer _sky;
        
    protected:

        IDToIndices _idToIndices;

        core::Bounds _bounds;
    };

}