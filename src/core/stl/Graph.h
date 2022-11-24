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

#define ADD_IN(name, t)
    
namespace core {

    class Graph {
    public:
        using id = int32_t;
        const static id NULL_ID = -1;

        struct Edge {
            id from = NULL_ID;
            id to = NULL_ID;
        };
        using Edges = std::vector<Edge>;

        enum DataType : int8_t
        {
            Scalar = 0,
            Vec2,
            Vec3,
            Vec4,
            
            Image,

            Buffer,


        };

        enum PinType : int8_t
        {
            in = 0,
            out,
            setting,

            count
        };

        struct PinClass {
            const std::string name;
            PinType           type = in;
            DataType          dataType = Scalar;
        };
        using PinClasses = std::vector<PinClass>;

        struct Pin {
            id _id;
            Edges   edges;

            bool connectTo(id to) {
                edges.push_back({ _id, to });
            }
        };
        using Pins = std::vector<Pin>;

        struct NodeSchema;
        using NodeSchemaPointer = std::shared_ptr<NodeSchema>;

        class Node {
            NodeSchema* _schema;
            
            friend Graph;
            void setup(NodeSchema* schema);
        public:
            Pins ins;
            Pins outs;
            Pins settings;

        };
        using NodePointer = std::shared_ptr<Node>;

        using Nodes = std::vector<NodePointer>;

        using NodeFunction = void (*)(Node& n);

        struct NodeSchema {
            virtual ~NodeSchema() {}

            PinClasses ins;
            PinClasses outs;
            PinClasses settings;
            std::string name;
        };


        NodePointer createNode(NodeSchema* NodeSchema);

        
        static void testGraph();

        Nodes _nodes;
    };
}
