// FileWatcher.h 
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
#pragma once

#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>
#include <memory>
#include <future>

namespace core {

// Define available file changes
enum class FileStatus { created, modified, erased };

class FileWatcher {
public:
    // Time interval at which we check the base folder for changes
    std::chrono::duration<int, std::milli> _delay;

    std::mutex _watchMapMutex;

    std::unique_ptr<std::thread> _watchThread;
    std::promise<void> _exitSignal;

    using Action = std::function<void(const std::string&, FileStatus)>;

    struct Entry {
        bool exists{ true };
        std::filesystem::file_time_type timestamp;
        FileWatcher::Action action;
    };

    // Keep a record of files from the base directory and their last modification time
    FileWatcher(std::chrono::duration<int, std::milli> delay);
    ~FileWatcher();

    static void watchThreadFunction(std::future<void> future, FileWatcher* fw);

    void watchFile(const std::string& path, const Action& action);
    void unwatchFile(const std::string& path);\

private:
    std::unordered_map<std::string, Entry> _watchMap;
    // Check if "paths_" contains a given key
    // If your compiler supports C++20 use paths_.contains(key) instead of this function
    bool contains(const std::string& key);
};

}
