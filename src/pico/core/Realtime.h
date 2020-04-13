// Realtime.h 
//
// Sam Gateau - April 2020
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

#include <chrono>

namespace core {

class FrameTimer {
public:
    struct Sample {
        uint32_t _frameNum { 0 };
        std::chrono::duration<uint32_t, std::micro> _frameDuration{ 1000000 };
        std::chrono::duration<uint32_t, std::micro> _frameBeginPeriod{ 1000000 };
        std::chrono::duration<uint32_t, std::micro> _frameEndPeriod{ 1000000 };

        float pureFrameRate() const { return (float)(1000000.0 / _frameDuration.count()); }
        float beginRate() const { return (float)(1000000.0 / _frameBeginPeriod.count()); }
        float endRate() const { return (float)(1000000.0 / _frameEndPeriod.count()); }
    };
    
    std::chrono::high_resolution_clock _clock;
    std::chrono::time_point<std::chrono::high_resolution_clock> _prevBeginTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> _prevEndTime;
    Sample _sample;
    bool _inPureFrame { false };

    Sample lastSample() const { return _sample; }

    void beginFrame() {
        if (!_inPureFrame) {
            auto newBeginTime = _clock.now();
            _sample._frameBeginPeriod = std::chrono::duration_cast<std::chrono::microseconds>(newBeginTime - _prevBeginTime);
            _prevBeginTime = newBeginTime;
            _sample._frameNum++;
            _inPureFrame = true;
        }
    }

    Sample endFrame() {
        if (_inPureFrame) {
            auto newEndTime = _clock.now();
            _sample._frameEndPeriod = std::chrono::duration_cast<std::chrono::microseconds>(newEndTime - _prevEndTime);
            _prevEndTime = newEndTime;
            _sample._frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(_prevEndTime - _prevBeginTime);
            _inPureFrame = false;
            return _sample;
        }
    }
};
}
