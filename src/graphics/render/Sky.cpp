// Sky.cpp
//
// Sam Gateau - January 2022
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
#include "Sky.h"

#include "gpu/Device.h"
#include "gpu/Resource.h"

using namespace graphics;

Sky::Sky() {

}

Sky::~Sky() {

}

#define useLocks 1
#ifdef useLocks 
#define WriteLock() const std::lock_guard<std::mutex> cpulock(_camData._access); _camData._version++;
#define ReadLock() const std::lock_guard<std::mutex> cpulock(_camData._access);

#define WriteGPULock() const std::lock_guard<std::mutex> gpulock(_gpuData._access); _gpuData._version++;
#define ReadGPULock() const std::lock_guard<std::mutex> gpulock(_gpuData._access);
#else
#define WriteLock()  _camData._version++;
#define ReadLock() 

#define WriteGPULock() _gpuData._version++;
#define ReadGPULock()
#endif

void Sky::setSunDir(const float3& dir) {
    WriteLock();
    _camData._data._sunDirection = core::normalize(dir);
}
float3 Sky::getSunDir() const {
    ReadLock();
    return _camData._data._sunDirection;
}

void Sky::setStageAltitude(float alt) {
    WriteLock();
    _camData._data._stageRT._columns[3].y = alt;
}
float Sky::getStageAltitude() const {
    ReadLock();
    return _camData._data._stageRT._columns[3].y;
}

void Sky::setSimDim(const int4& dims) {
    WriteLock();
    _camData._data._simDim = dims;
}

int4 Sky::getSimDim() const {
    ReadLock();
    return _camData._data._simDim;
}


void Sky::allocateGPUData(const DevicePointer& device) {
    // CReate a gpu buffer to hold the Sky
    graphics::BufferInit uboInit;
    uboInit.usage = ResourceUsage::UNIFORM_BUFFER;
    uboInit.bufferSize = sizeof(SkyData);
    uboInit.hostVisible = true;
    _gpuData._buffer = device->createBuffer(uboInit);

    // and then copy data there
    ReadLock();
    memcpy(_gpuData._buffer->_cpuMappedAddress, &_camData._data, sizeof(SkyData));

    // sync the data version
    _gpuData._version = _camData._version;
}

bool Sky::updateGPUData() {
    // TODO make the version an atomic to be thread safe...
    bool needCopy = (_gpuData._version != _camData._version);
    // copy data from tcpu to gpu here if versions are different
    if (needCopy) {
        ReadLock();
        WriteGPULock();
        memcpy(_gpuData._buffer->_cpuMappedAddress, &_camData._data, sizeof(SkyData));

        // sync the data version
        _gpuData._version = _camData._version;
    }
    return needCopy;
}

BufferPointer Sky::getGPUBuffer() const {
    ReadGPULock();
    return _gpuData._buffer;
}

