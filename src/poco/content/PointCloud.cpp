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

PointCloud::PointCloud(const PointCloudInit& init) {

}

PointCloud::~PointCloud() {

}

PointCloudPointer PointCloud::createFromPLY(const std::string& filename) {

    if (filename.empty()) {
        return nullptr;
    }

    // let's try to open the file
    std::ifstream file(filename, std::ifstream::in );

    std::stringstream header;
    header << file.rdbuf();
    std::string token;
 
    // ply
    header >> token;
    if (token.compare("ply") != 0) {
        return nullptr;
    }

    // Now grab every lane and then read the first token to identify the information
    // when we reach  end_header, then the rest of the stream is the inary info.
    bool headerRead = false;

    std::vector<std::pair<std::string, uint32_t>> elements;
    std::vector<std::pair<std::string, std::string>> properties;

    const uint32_t lineLength = 512;
    char line[lineLength];
    while (!header.eof()) {
        header.getline(line, lineLength);
        std::stringstream lineStream;
        lineStream << line;
        
        lineStream >> token;
        if (token.compare("end_header") == 0) {
            headerRead = true;
            break;
        }
        else if (token.compare("format") == 0) {
            std::string format;
            float formatVal;
            lineStream >> format;
            lineStream >> formatVal;
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
            elements.push_back({ element, numElements });
        }
        else if (token.compare("property") == 0) {
            std::string type, name;
            lineStream >> type;
            lineStream >> name;
            properties.push_back({ type, name });
        }
    }
 
    return nullptr;
}


