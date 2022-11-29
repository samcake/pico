// Graph.h 
//
// Sam Gateau - October 2022
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
#include <stdint.h>
#include <vector>
#include <string>
#include <array>
#include <memory>
#include <functional>
#include <set>

#define STR1(x)  #x
#define STR(x)  STR1(x)
#define ADD_IN(name, t) { STR(name), { PinAccess::in, DataType::t }}
#define ADD_OUT(name, t) { STR(name), { PinAccess::out, DataType::t }}
    
namespace core {

 //   class Graph;
 //   using GraphPointer = std::shared_ptr<Graph>;

    class Graph {
    public:
        using Name = const std::string;
        using Hash = size_t;

        using Index = int32_t;
        using ID = Index;
        using NodeID = Index;
        using PinID = Index;
        using EdgeID = Index;
        const static ID NULL_ID = -1;



        // Describing the Pin Type / Schema
        enum DataType : int8_t
        {
            Scalar = 0,
            Vec2,
            Vec3,
            Vec4,
            
            Image,

            Buffer,


        };

        enum PinAccess : int8_t
        {
            in = 0,
            out,
            setting,

            count
        };
        
        struct PinType {
            PinAccess         access = in;
            DataType          dataType = Scalar;

            Hash hash() const { return std::hash<int16_t>{}((int16_t)access + 256 * (int16_t)dataType); }
        };
        using PinTypes = std::vector<PinType>;

        struct NamedPinType
        {
            Name name;
            PinType type;

            static Hash hash(const NamedPinType& npt)
            {
                auto v = std::hash<std::string>{}(npt.name);
                v += npt.type.hash();
                return std::hash<Hash>{}(v);
            }
            Hash hash() const { return hash(*this); }
        };
        using NamedPinTypes = std::vector<NamedPinType>;

        

        struct NamedPinTypeSet
        {
            NamedPinTypes pins;
            
            Index operator[] (Name& name) const
            {
                Index i = 0;
                for (const auto& it : pins)
                {
                    if (it.name.compare(name) == 0)
                        return i;

                    ++i;
                }
                return -1;
            }

            Index operator[] (Hash hash) const
            {
                Index i = 0;
                for (const auto& it : pins)
                {
                    if (it.hash() == hash)
                        return i;
                    ++i;
                }
                return -1;
            }

            Hash hash() const
            {
                Hash v = 0;
                for (const auto& it : pins)
                {
                    v += it.hash();
                }
                return std::hash<Hash>{}(v);
            }
        };

        // Describing the Node Type as a name + collection of pin type sets
        struct NodeType {

            struct Init {
                Name name;
                NamedPinTypes ins;
                NamedPinTypes outs;
            };

            Name name;
            NamedPinTypeSet in_pins;
            NamedPinTypeSet out_pins;

            NodeType(const Init& init) :
                name(init.name),
                in_pins({ init.ins }),
                out_pins({init.outs }) {}

            Hash hash() const
            {
                auto v = std::hash<std::string>{}(name);
                v += in_pins.hash();
                v += out_pins.hash();
                return std::hash<Hash>{}(v);
            };

            auto pin(Hash hash) const {
                auto i = in_pins[hash];
                if (i != NULL_ID)
                    return std::pair(i, PinAccess::in);
                i = out_pins[hash];
                if (i != NULL_ID)
                    return std::pair(i, PinAccess::out);

                return std::pair(NULL_ID, PinAccess::count);
            }

            Name pinName(Hash hash) const {
                auto i = pin(hash);
                if (i.first != NULL_ID)
                {
                    if (i.second == PinAccess::in)
                        return in_pins.pins[i.first].name;
                    else
                        return out_pins.pins[i.first].name;
                }
                return "";
            }

            PinType pinType(Hash hash) const {
                auto i = pin(hash);
                if (i.first != NULL_ID)
                {
                    if (i.second == PinAccess::in)
                        return in_pins.pins[i.first].type;
                    else
                        return out_pins.pins[i.first].type;
                }
                return PinType();
            }

        };

        using IDs = std::vector<ID>;
        using NodeIDs = std::vector<NodeID>;
        using PinIDs = std::vector<PinID>;
        using EdgeIDs = std::vector<EdgeID>;

        class Node;
        struct Pin {
            Node*   node = nullptr;
            Hash    named_type_hash = 0;
            PinID   uuid = 0;

            EdgeIDs  edges;

            Name    name() const { return node->_type->pinName(named_type_hash); }
            PinType pinType() const { return node->_type->pinType(named_type_hash); }
        };

        using Pins = std::vector<Pin>;

        struct Edge {
            PinID from = NULL_ID;
            PinID to = NULL_ID;

            EdgeID uuid = NULL_ID;
        };

        using Edges = std::vector<Edge>;


        using NodeTypePointer = std::shared_ptr<NodeType>;

        class Node {
            NodeTypePointer _type;
            
            friend Graph;
            Graph* _graph;
        public:
          //  const ID operator[] (Name& name) const { return (*const_cast<Node*>(this))[name]; }

            PinID in_pin_id(Name& name) const {
                auto i = _type->in_pins[name];
                if (i >= 0)
                {
                    return ins[i];
                }
                return NULL_ID;
            }
            PinID out_pin_id(Name& name) const {
                auto i = _type->out_pins[name];
                if (i >= 0)
                {
                    return outs[i];
                }
                return NULL_ID;
            }

            Pin* in_pin(Name& name) const {
                return _graph->pin(in_pin_id(name));
            }
            Pin* out_pin(Name& name) const {
                return _graph->pin(out_pin_id(name));
            }


            PinIDs ins;
            PinIDs outs;
            NodeID uuid = NULL_ID;

            void log();

        };
        using NodePointer = std::shared_ptr<Node>;

        using Nodes = std::vector<NodePointer>;

        using NodeFunction = void (*)(Node& n);


        /// Graph methods to build the graph!

        // Create a node
        NodePointer createNode(const NodeTypePointer& nodeType);

        EdgeID connect(const NodePointer& nodeFrom, Name& pinFrom, const NodePointer& nodeTo, Name& pinTo)
        {
            return connect(*nodeFrom, pinFrom, *nodeTo, pinTo);
        }
        EdgeID connect(Node& nodeFrom, Name& pinFrom, Node& nodeTo, Name& pinTo);
        
        Pin* pin(PinID uuid) { return (uuid == NULL_ID ? nullptr : _pins.data() + uuid); }


        class NodeGraph : public Node
        {
            Graph _subgraph;
        public:

        };

        using NodeGraphPointer = std::shared_ptr<NodeGraph>;

        NodeGraphPointer createNodeGraph(const Graph& subgraph);

        static void testGraph();

        void log() const;


        Nodes _nodes;
        Pins _pins;
        Edges _edges;

        mutable IDs _inPins;
        mutable IDs _outPins;
        
        mutable IDs _traverseOrder;

        mutable int32_t _inNodesCount = 0; // number of starting nodes in the traverse order
        mutable int32_t _outNodesCount = 0; // number of ending nodes in the traverse order
        mutable int32_t _nodeWavesCount = 0; // number of waves to go through the graph

        NodeIDs gatherSourceNodes(const Node* n) const;
        void gatherOuterNodes(const std::vector<NodeIDs>& sourceNodesPerNode, std::set<ID>& nodeReservoirA, std::set<ID>& nodeReservoirB) const;
        void evalTraverseOrder() const;
        void evalInOutPins() const;

        void traverse(std::function<void (Node*, int32_t)> visitor) const;
    };
}
