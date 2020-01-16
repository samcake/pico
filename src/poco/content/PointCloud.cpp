// PointCloud.cpp
//
// Sam Gateau - 2020/1/1
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
#include "PointCloud.h"

#include <fstream>
#include <sstream>

#include <vector>

using namespace poco;

PointCloud::PointCloud(const PointCloudInit& init) : 
    _mesh(init.mesh)
{

}

PointCloud::~PointCloud() {

}

PointCloudPointer PointCloud::createFromPLY(const std::string& filename) {

    if (filename.empty()) {
        return nullptr;
    }

    // let's try to open the file
    std::ifstream file(filename, std::ifstream::in );
    std::stringstream asciiStream;
    asciiStream << file.rdbuf();
    file.close();

    { // ply
        std::string token;
        asciiStream >> token;
        if (token.compare("ply") != 0) {
            return nullptr;
        }
    }

    // Now grab every lane and then read the first token to identify the information
    // when we reach  end_header, then the rest of the stream is the inary info.
    bool headerOver = false;

    struct Type {
        enum Name {
            _uchar = 0,
            _int,
            _float,
            unknown
        };
        static Name fromString(const std::string& token) {
            if (token.compare("uchar") == 0) {
                return Name::_uchar;
            }
            else if (token.compare("int") == 0) {
                return Name::_int;
            }
            else if (token.compare("float") == 0) {
                return Name::_float;
            }
            return Name::unknown;
        }

        Name name{ unknown };

        Type() {}
        Type(const std::string& str) : name(fromString(str)) {}
    };

    struct Format {
        enum Name {
            ascii = 0,
            binary_little_endian,
            binary_big_endian,
            unknown
        };
        static Name fromString(const std::string& token) {
            if (token.compare("ascii") == 0) {
                return Format::ascii;
            }
            else if (token.compare("binary_little_endian") == 0) {
                return Format::binary_little_endian;
            }
            else if (token.compare("binary_big_endian") == 0) {
                return Format::binary_big_endian;
            }
            return Format::unknown;
        }

        Name name;
        Format() {}
        Format(const std::string& str) : name(fromString(str)) {}
    };
    
    struct Element {
        std::string name;
        uint32_t count;
    };
    struct Property {
        std::string name;
        Type type;
        Type countType;
        bool isList{ false };

        Property(const std::string& n, Type t) : name(n), type(t) {}
        Property(const std::string& n, Type t, Type ct) : name(n), type(t), countType(ct), isList(true) {}
    };
    using Properties = std::vector<Property>;

    Format format;
    float formatVersion{ 0.0f };
    std::vector<std::pair<Element, Properties>> elements;

    const uint32_t lineLength = 512;
    char line[lineLength];
    while (!asciiStream.eof()) {
        asciiStream.getline(line, lineLength);
        std::stringstream lineStream;
        lineStream << line;

        std::string token;
        lineStream >> token;
        if (token.compare("end_header") == 0) {
            headerOver = true;
            break;
        }
        else if (token.compare("format") == 0) {
            std::string formatStr;
            float formatVal;
            lineStream >> formatStr;
            lineStream >> formatVal;
            format.name = Format::fromString(formatStr);
            formatVersion = formatVal;
        }
        else if (token.compare("comment") == 0) {
            std::string comment;
            lineStream >> comment;
        }
        else if (token.compare("element") == 0) {
            std::string element;
            uint32_t numElements;
            lineStream >> element;
            lineStream >> numElements;
            elements.push_back({{ element, numElements }, Properties()});
        }
        else if (token.compare("property") == 0) {
            std::string isList_or_type, name, counterType;
            lineStream >> isList_or_type;
            if (isList_or_type.compare("list") == 0) {
                std::string counterType;
                lineStream >> counterType;
                lineStream >> isList_or_type;
                lineStream >> name;
                elements.back().second.push_back({ name, Type(isList_or_type), Type(counterType) });
            } else {
                lineStream >> name;
                elements.back().second.push_back({ name, Type(isList_or_type) });
            }
        }
    }

    if (!headerOver) {
        // Header wasn't read to conclusion... early exit
        return nullptr;
    }
 
    // Now let's grab the rest of the file
    size_t beginPayloadPos = asciiStream.tellg();

    // REopen the file in binary mode to grab the content part
    std::ifstream bfile(filename, std::ifstream::in | std::ios::binary);
    std::vector<char> fileBinaries((std::istreambuf_iterator<char>(bfile)), std::istreambuf_iterator<char>());
    bfile.close();

    auto contentSize = fileBinaries.size() - beginPayloadPos;
    auto contentBegin = fileBinaries.data() + beginPayloadPos;

    struct Vertex {
        vec3 pos;
        vec3 nor;
        ucvec4 color;
    };
    size_t verticesByteSize = elements[0].first.count * sizeof(Vertex);


  //  std::vector< Vertex > vertices;
  //  vertices.resize(elements[0].first.count);

    AttribBufferPointer vertices;


    switch (format.name) {
    case Format::binary_little_endian: {
        vertices = std::make_shared<AttribBuffer>((void*)contentBegin, verticesByteSize);
    } break;
    case Format::binary_big_endian:
    case Format::ascii:
    case Format::unknown:
        return nullptr;
    break;
    }

    auto vertexFormat = new StreamAccessorInstanced<3, 1>();

    vertexFormat->attribs[0] = { AttribFormat::VEC3, 0, 0 };
    vertexFormat->attribs[1] = { AttribFormat::VEC3, 0, 12 };
    vertexFormat->attribs[2] = { AttribFormat::CVEC4, 0, 24 };

    vertexFormat->bufferViews[0] = {0, 28, 0, 0xFFFFFFFF};

    auto mesh = std::make_shared<Mesh>();

    mesh->_vertexBuffers._accessor = vertexFormat;
    mesh->_vertexBuffers._buffers.push_back(vertices);


    PointCloudInit pcinit{ mesh };
    return std::make_shared<PointCloud>(pcinit);

}


