// PointCloud.cpp
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
#include "PointCloud.h"

#include <fstream>
#include <sstream>

#include <vector>

// using namespace pico;
namespace document
{

    PointCloud::PointCloud()
    {

    }

    PointCloud::~PointCloud() {

    }

    PointCloudPointer PointCloud::createFromPLY(const std::string& filename) {

        if (filename.empty()) {
            return nullptr;
        }

        // let's try to open the file
        std::ifstream file(filename, std::ifstream::in);
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
                unknown,
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

            static uint32_t sizeOf(Name name) {
                static const uint32_t sizes[] = { 1, 4, 4, 0 }; // size for each type
                return sizes[(int)name];
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
                elements.push_back({ { element, numElements }, Properties() });
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
                }
                else {
                    lineStream >> name;
                    elements.back().second.push_back({ name, Type(isList_or_type) });
                }
            }
        }

        if (!headerOver) {
            // Header wasn't read to conclusion... early exit
            return nullptr;
        }


        //analyze the vertex format described
        // look for the vertex element:
        int vertexElementIndex = -1;
        int indexElementIndex = -1;
        for (int i = 0; i < elements.size(); ++i) {
            const auto& element = elements[i];
            // detect vertex element
            if ((vertexElementIndex < 0) && element.first.name.compare("vertex") == 0) {
                vertexElementIndex = i;
            }
            // detect vertex element
            if ((indexElementIndex < 0) && element.first.name.compare("face") == 0) {
                indexElementIndex = i;
            }
        }

        if ((vertexElementIndex < 0)) {
            // No vertex found ??? exit
            return nullptr;
        }
        // Let's understand the vertex attributes layout
        const auto& vertexElement = elements[vertexElementIndex];
        auto numVertProperties = vertexElement.second.size();
        auto numVertices = vertexElement.first.count;
        int vertexAttribOffset = 0;
        std::vector<uint32_t> propertyOffsets(numVertProperties + 1);
        for (int a = 0; a < numVertProperties; ++a) {
            const auto* prop = vertexElement.second.data() + a;
            propertyOffsets[a] = vertexAttribOffset;
            vertexAttribOffset += Type::sizeOf(prop->type.name);
        }
        propertyOffsets[numVertProperties] = vertexAttribOffset;


        int vertexPositionPropIndex = -1;
        int vertexNormalPropIndex = -1;
        int vertexColorPropIndex = -1;
        int vertexAlphaPropIndex = -1;
        
        int vertexPositionByteOffset = -1;
        int vertexNormalByteOffset = -1;
        int vertexColorByteOffset = -1;
        int vertexAlphaByteOffset = -1;

        int vertexPositionByteSize =-1;
        int vertexNormalByteSize = -1;
        int vertexColorByteSize = -1;
        int vertexAlphaByteSize = -1;

        for (int a = 0; a < numVertProperties; ++a) {
            const auto* prop = vertexElement.second.data() + a;
            // detect position with 'x'
            if (prop->name.compare("x") == 0) {
                // Check that:
                if (
                    // there are 2 more properties in front of us
                    ((a + 2) < numVertProperties)
                    // called 'y' and 'z'
                    && ((prop + 1)->name.compare("y") == 0) && ((prop + 2)->name.compare("z") == 0)
                    // of type float
                    && (prop->type.name == Type::_float) && ((prop + 1)->type.name == Type::_float) && ((prop + 2)->type.name == Type::_float)
                    ) {
                    // this is the vertex position 
                    vertexPositionPropIndex = a;
                    vertexPositionByteOffset = propertyOffsets[vertexPositionPropIndex];
                    vertexPositionByteSize = propertyOffsets[vertexPositionPropIndex + 3] - vertexPositionByteOffset;
                }
            }
            // detect normal with 'nx'
            else if (prop->name.compare("nx") == 0) {
                // Check that:
                if (
                    // there are 2 more properties in front of us
                    ((a + 2) < numVertProperties)
                    // called 'ny' and 'nz'
                    && ((prop + 1)->name.compare("ny") == 0) && ((prop + 2)->name.compare("nz") == 0)
                    // of type float
                    && (prop->type.name == Type::_float) && ((prop + 1)->type.name == Type::_float) && ((prop + 2)->type.name == Type::_float)
                    ) {
                    // this is the vertex normal 
                    vertexNormalPropIndex = a;
                    vertexNormalByteOffset = propertyOffsets[vertexNormalPropIndex];
                    vertexNormalByteSize = propertyOffsets[vertexNormalPropIndex + 3] - vertexNormalByteOffset;

                }
            }
            // detect color with 'red'
            else if (prop->name.compare("red") == 0) {
                // Check that:
                if (
                    // there are 2 more properties in front of us
                    ((a + 2) < numVertProperties)
                    // called 'green' and 'blue'
                    && ((prop + 1)->name.compare("green") == 0) && ((prop + 2)->name.compare("blue") == 0)
                    // of same type uchar
                    && (prop->type.name == Type::_uchar) && ((prop + 1)->type.name == Type::_uchar) && ((prop + 2)->type.name == Type::_uchar)
                    ) {
                    // this is the vertex color 
                    vertexColorPropIndex = a;
                    vertexColorByteOffset = propertyOffsets[vertexColorPropIndex];
                    vertexColorByteSize = propertyOffsets[vertexColorPropIndex + 3] - vertexColorByteOffset;


                    // if the 4th prop exists and is called alpha then bingo
                    if (
                        ((a + 3) < numVertProperties)
                        // called 'alpha'
                        && ((prop + 3)->name.compare("alpha") == 0)
                        // of type uchar
                        && ((prop + 3)->type.name == Type::_uchar)
                        ) {
                        // this is the vertex alpha
                        vertexAlphaPropIndex = a + 3;
                        vertexAlphaByteOffset = propertyOffsets[vertexAlphaPropIndex];
                        vertexAlphaByteSize = propertyOffsets[vertexAlphaPropIndex + 1] - vertexAlphaByteOffset;
                        vertexColorByteSize += vertexAlphaByteSize; // We adjust the color byte size adding the A
                    }
                }
            }
        }

        // No vertex position found, exit
        if (vertexPositionPropIndex < 0) {
            return nullptr;
        }

        // Unknwon data format, exit
        if (format.name == Format::unknown) {
            return nullptr;
        }

        // Let's allocate the Points
        Points points;
        size_t verticesByteSize = numVertices * sizeof(Point);


        if (format.name == Format::ascii) {
            // allocate the vertex size for the expected size
            points.resize(numVertices);
            auto vert = reinterpret_cast<Point*>(points.data());
            // let's parse the vertices walking through the stream
            unsigned int v = 0;
            while ((v < numVertices) && !asciiStream.eof()) {
                asciiStream >> vert->pos.x >> vert->pos.y >> vert->pos.z;
                if (vertexNormalPropIndex > 0) {
                    core::vec3 nor;
                    asciiStream >> nor.x >> nor.y >> nor.z;
                    // vert->nor = nor;
                }
                else {
                    //vert->nor = core::vec3();
                }
                if (vertexColorPropIndex > 0) {
                    int r, g, b, a(255);
                    asciiStream >> r >> g >> b;
                    if (vertexAlphaPropIndex > 0) {
                        asciiStream >> a;
                    }
                    vert->color = core::ucvec4(r, g, b, a);
                }
                else {
                    vert->color = core::ucvec4(255);
                }
                v++;
                vert++;
            }

            // success probably?
            if (v <= numVertices) {
                if (v < numVertices) {
                    numVertices = v;
                    points.resize(numVertices);
                }
            }
            else {
                // something went wrong ?
                return nullptr;
            }
        }
        else {
            // binary data let's grab the rest of the file
            size_t beginPayloadPos = asciiStream.tellg();

            // REopen the file in binary mode to grab the content part
            std::ifstream bfile(filename, std::ifstream::in | std::ios::binary);
            std::vector<char> fileBinaries((std::istreambuf_iterator<char>(bfile)), std::istreambuf_iterator<char>());
            bfile.close();

            auto contentSize = fileBinaries.size() - beginPayloadPos;
            auto contentBegin = fileBinaries.data() + beginPayloadPos;

            uint32_t vertexByteStride = (uint32_t)(contentSize) / numVertices;

            // Understand format layout and binary layout
            bool formatLayoutMatch = (verticesByteSize == contentSize) && (vertexPositionPropIndex == 0) && (vertexColorPropIndex == 3) && (vertexAlphaPropIndex == 6);
            bool binaryBigEndian = (format.name == Format::binary_big_endian);

            // THe fast path is when the binary format is exactly what we need for rendering:
            if (formatLayoutMatch && format.name == Format::binary_little_endian) {
                points.resize(numVertices);
                memcpy(points.data(), (void*)contentBegin, verticesByteSize);
            } else {
                // The slow path, just like with ascii, we need to read attributes one by one
  
                // allocate the vertex size for the expected size
                points.resize(numVertices);
                auto vert = reinterpret_cast<Point*>(points.data());
                // let's parse the vertices walking through the stream
                uint32_t v = 0;

                auto currentVertByteOffset = contentBegin;

                for (; v < numVertices; v++) {
                    currentVertByteOffset = contentBegin + v * vertexByteStride;

                    // assume pos is first, no way around
                    memcpy(vert->pos.data(), currentVertByteOffset + vertexPositionByteOffset, vertexPositionByteSize);

                    if (vertexNormalPropIndex > 0) {
                        core::vec3 nor;
                        memcpy(nor.data(), currentVertByteOffset + vertexNormalByteOffset, vertexNormalByteSize);
                        // vert->nor = nor;
                    }
                    else {
                        //vert->nor = core::vec3();
                    }
                    if (vertexColorPropIndex > 0) {
                        memcpy(vert->color.data(), currentVertByteOffset + vertexColorByteOffset, vertexColorByteSize);

                        if (vertexAlphaPropIndex > 0) {
                        } else {
                            vert->color.w = 255;
                        }
                    }
                    else {
                        vert->color = core::ucvec4(255);
                    }
                    vert++;
                }

                // success probably?
                if (v <= numVertices) {
                    if (v < numVertices) {
                        numVertices = v;
                        points.resize(numVertices);
                    }
                }
                else {
                    // something went wrong ?
                    return nullptr;
                }

            }
        }

        auto pointCloud = std::make_shared<PointCloud>();
        pointCloud->_points = points;
        /*    pointCloud->_points = points;
            pointCloud->_points = points;
            pointCloud->_points = points;
        */
        return pointCloud;


    }


}  // !namespace document