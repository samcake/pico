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
#include <core/math/LinearAlgebra.h>

#include "Renderer.h"
#include "render/Transform.h"

namespace graphics {
  
    using NodeID = TransformTree::NodeID;
    const static NodeID INVALID_NODE_ID {TransformTree::INVALID_ID};

    class VISUALIZATION_API Node {
        Node() { } // invalid
        friend class Scene;
        Node(TransformTreeGPUPointer transformTree, NodeID index) : _transformTree(transformTree), _index(index) { }

        TransformTreeGPUPointer _transformTree;
        NodeID _index;
    public:

        Node(const Node& node) : _transformTree(node._transformTree), _index(node._index) {}

        TransformTreeGPUPointer tree() const { return _transformTree; }
        NodeID id() const { return _index; }
    };

    template <typename T> void log(const T& x, std::ostream& out, size_t position) {
        out << std::string(position, ' ') << std::endl;
    }

    template <typename T> DrawcallObjectPointer item_getDrawable(const T& x) {
        return x->getDrawable();
    }

    using ItemID = uint32_t;

    class VISUALIZATION_API Item {
        Item() { } // invalid
        friend class Scene;
    public:
        static const ItemID INVALID_ITEM_ID{ 0xFFFFFFFF };
        static Item null;

        template <typename T> Item(uint32_t index, T x):
            _self(std::make_shared<Model<T>>(index, std::move(x))) {
        }

        friend void log(const Item& x, std::ostream& out, size_t position) {
            x._self->_log(out, position);
        }

        DrawcallObjectPointer getDrawable() const { 
            return _self->_getDrawable();
        }
            
        friend DrawcallObjectPointer getDrawable(const Item& x) {
            return x._self->_getDrawable();
        }

        bool isValid() const { return (_self != nullptr); }

        uint32_t getIndex() const { return  _self->getIndex(); }

        bool isVisible() const { return _self->isVisible(); }
        void setVisible(bool visible) { _self->setVisible(visible); }

        void setNode(Node node) { _self->setNode(node); }
        NodeID getNode() const { return _self->getNode(); }

    private:
        struct Concept{
            Concept(uint32_t index) : _index(index) {}
            virtual ~Concept() = default;
            virtual void _log(std::ostream& out, size_t position) const = 0;
            virtual DrawcallObjectPointer _getDrawable() const = 0;

            uint32_t getIndex() const { return _index; }

            void setNode(Node node) const;
            NodeID getNode() const { return _nodeID; }

            bool isVisible() const { return _isVisible; }
            void setVisible(bool visible) const { _isVisible = visible; }

            const uint32_t _index;
            mutable NodeID _nodeID{ INVALID_NODE_ID };
            mutable bool _isVisible { true };
        };
        template <typename T> struct Model final : Concept {
             Model(uint32_t index, T x) : Concept(index), _data(std::move(x)) {}

             void _log(std::ostream& out, size_t position) const override {
                log(_data, out, position);
             }

             DrawcallObjectPointer _getDrawable() const override {
                 return item_getDrawable(_data);
             }
            
             T _data;
        };

        std::shared_ptr<const Concept> _self;
    };
    using Items = std::vector<Item>;
    using IDToIndices = std::unordered_map<ItemID, uint32_t>;



    class VISUALIZATION_API Scene {
    public:
        Scene();
        ~Scene();


        template <typename T>
        Item createItem(T& x, ItemID userID = Item::INVALID_ITEM_ID) {
            return _createItem(Item((uint32_t)_items.size(), x), userID);
        }

        void deleteAllItems();  // delete all user objects
        void deleteAll();
        Item deleteItem(ItemID id);

        Item getItemFromID(ItemID id) const;
        const Items& getItems() const { return _items; } // THe all items, beware this contains  INVALID items
        const Item& getValidItemAt(uint32_t startIndex) const;

        const core::Bounds& getBounds() const { return _bounds; }


        // Nodes
        Node createNode(const core::mat4x3& rts, NodeID parent);
        void deleteNode(NodeID nodeId);

        void attachNode(NodeID child, NodeID parent);
        void detachNode(NodeID child);

        TransformTreeGPUPointer _transformTree;

    protected:

        Items _items;
        IDToIndices _idToIndices;

        core::Bounds _bounds;

        Item _createItem(Item& newItem, ItemID userID);
    };

}