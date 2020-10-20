// FileWatcher.cpp 
//
// Sam Gateau - October 2020
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
#include "FileWatcher.h"

#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>
#include <memory>
#include <future>
#include <fstream>
#include <sstream>

using namespace core;

// Keep a record of files from the base directory and their last modification time
FileWatcher::FileWatcher(std::chrono::duration<int, std::milli> delay) : _delay{ delay } {

    //Fetch std::future object associated with promise
    std::future<void> futureObj = _exitSignal.get_future();

    // Starting Thread & move the future object in lambda function by reference
    _watchThread.reset(new std::thread(&FileWatcher::watchThreadFunction, std::move(futureObj), (this)));
}

FileWatcher::~FileWatcher() {
    //Set the value in promise
    _exitSignal.set_value();

    //Wait for thread to join
    _watchThread->join();
}

void FileWatcher::watchThreadFunction(std::future<void> future, FileWatcher* fw) {
    while (future.wait_for(fw->_delay) == std::future_status::timeout) {

        {
            std::lock_guard<std::mutex> lock(fw->_watchMapMutex);

            auto it = fw->_watchMap.begin();
            while (it != fw->_watchMap.end()) {
                auto& entry = (*it).second;
                bool nowExists = std::filesystem::exists(it->first);

                if (!entry.exists) {
                    if (nowExists) {
                        entry.exists = true;
                        entry.timestamp = std::filesystem::last_write_time(it->first);
                        entry.action(it->first, FileStatus::created);
                    }
                }
                else {
                    if (!nowExists) {
                        entry.exists = false;
                        // entry.timestamp = 0;
                        entry.action(it->first, FileStatus::erased);
                    }
                    else {
                        auto lastTimeStamp = std::filesystem::last_write_time(it->first);
                        if (entry.timestamp < lastTimeStamp) {
                            entry.timestamp = lastTimeStamp;
                            entry.action(it->first, FileStatus::modified);
                        }
                    }
                }
                it++;
            }
        }
    }

}

void FileWatcher::watchFile(const std::string& path, const Action& action) {
    std::lock_guard<std::mutex> lock(_watchMapMutex);
    _watchMap[path] = { std::filesystem::exists(path), std::filesystem::last_write_time(path), action };
}

void FileWatcher::unwatchFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(_watchMapMutex);
    if (contains(path)) {
        _watchMap.erase(_watchMap.find(path));
    }
}

// Check if "paths_" contains a given key
// If your compiler supports C++20 use paths_.contains(key) instead of this function
bool FileWatcher::contains(const std::string& key) {
    auto el = _watchMap.find(key);
    return el != _watchMap.end();
}