// Job.cpp
//
// Sam Gateau - April 2026
//
// MIT License
//
// Copyright (c) 2026 Sam Gateau
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
#include "Job.h"

#include <chrono>

#include "Log.h"

#ifdef _WIN32
#include <windows.h>
static void setThreadName(const std::string& name) {
    std::wstring wide(name.begin(), name.end());
    SetThreadDescription(GetCurrentThread(), wide.c_str());
}
static std::string threadName(uint32_t i) {
    if (i < 26)
        return std::string(1, char('A' + i));
    i -= 26;
    return std::string(1, char('A' + i / 26)) + char('A' + i % 26);
}
#else
static void setThreadName(const std::string&) {}
#endif

namespace core {

    // -------------------------------------------------------------------------
    // ThreadPool
    // -------------------------------------------------------------------------

    ThreadPool::ThreadPool(uint32_t num_threads) {
        for (uint32_t i = 0; i < num_threads; ++i) {
            _workers.emplace_back([this, i] {
                setThreadName("pico::Job::Thread::" + threadName(i));
                while (true) {
                    std::function<void()> fn;
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        _cv.wait(lock, [this] { return _stop || !_queue.empty(); });
                        if (_stop && _queue.empty()) return;
                        fn = std::move(_queue.front());
                        _queue.pop_front();
                    }
                    try {
                        fn();
                    } catch (const std::exception& e) {
                        picoLogf("ThreadPool: uncaught exception in job: {}", e.what());
                    } catch (...) {
                        picoLog("ThreadPool: unknown uncaught exception in job");
                    }
                }
            });
        }
    }

    ThreadPool::~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _stop = true;
        }
        _cv.notify_all();
        for (auto& w : _workers)
            w.join();
    }

    void ThreadPool::enqueue(std::function<void()> fn) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _queue.push_back(std::move(fn));
        }
        _cv.notify_one();
    }

    // -------------------------------------------------------------------------
    // Job
    // -------------------------------------------------------------------------

    Job::Job(std::string_view name)
        : _name(name)
        , _graph(std::make_shared<JobGraph>())
    {
    }

    JobPtr Job::create(std::string_view name) {
        JobPtr job(new Job(name));
        job->_graph->addJob(job);
        return job;
    }

    void Job::execute() {
        // Mark all output slots not-ready before kernel runs
        for (auto& slot : _output_slots)
            slot->_ready.store(false, std::memory_order_relaxed);

        if (_kernel)
            _kernel();

        // Mark all output slots ready — release so consumers on other threads
        // see the written _value
        for (auto& slot : _output_slots)
            slot->_ready.store(true, std::memory_order_release);
    }

    // -------------------------------------------------------------------------
    // JobGraph
    // -------------------------------------------------------------------------

    void JobGraph::addJob(const JobPtr& job) {
        _jobs.push_back(job);
        _dirty = true;
    }

    void JobGraph::merge(const JobGraphPtr& other) {
        if (!other || other.get() == this) return;

        JobGraphPtr self = shared_from_this();

        std::vector<JobPtr> toMove = other->_jobs;
        other->_jobs.clear();

        for (auto& job : toMove) {
            job->_graph = self;
            _jobs.push_back(job);
        }

        _dirty = true;
    }

    void JobGraph::recomputeTopoOrder() {
        // Kahn's algorithm — also builds _dependents for multithreaded dispatch
        std::unordered_map<Job*, int> inDegree;

        _dependents.clear();

        for (auto& job : _jobs) {
            if (!inDegree.count(job.get()))
                inDegree[job.get()] = 0;
            for (auto& input : job->inputJobs()) {
                _dependents[input.get()].push_back(job);
                inDegree[job.get()]++;
            }
        }

        std::unordered_map<Job*, JobPtr> ptrToShared;
        for (auto& job : _jobs)
            ptrToShared[job.get()] = job;

        std::vector<Job*> queue;
        for (auto& [ptr, deg] : inDegree)
            if (deg == 0) queue.push_back(ptr);

        _topoOrder.clear();
        while (!queue.empty()) {
            Job* current = queue.back();
            queue.pop_back();
            _topoOrder.push_back(ptrToShared[current]);
            for (auto& dep : _dependents[current])
                if (--inDegree[dep.get()] == 0)
                    queue.push_back(dep.get());
        }

        if (_topoOrder.size() != _jobs.size()) {
            picoLog("JobGraph: cycle detected — execution order is incomplete");
            _topoOrder.clear();
        }

        _dirty = false;
    }

    void JobGraph::executeOnCurrentThread() {
        if (_dirty)
            recomputeTopoOrder();
        for (auto& job : _topoOrder)
            job->execute();
    }

    bool JobGraph::execute(ThreadPool& pool) {
        if (_dirty)
            recomputeTopoOrder();

        if (_topoOrder.empty() && !_jobs.empty())
            return false;  // cycle detected

        const int total = static_cast<int>(_jobs.size());

        // Reset pending counters for this execution
        for (auto& job : _jobs)
            job->_pending_inputs.store(static_cast<int>(job->inputJobs().size()),
                                       std::memory_order_relaxed);

        // Completion tracking
        std::mutex              doneMutex;
        std::condition_variable doneCV;
        std::atomic<int>        remaining { total };

        // Shared dispatch function — captures everything by reference
        // Safe: all captured refs live on this stack frame until doneCV fires
        std::function<void(const JobPtr&)> enqueueJob;
        enqueueJob = [&](const JobPtr& job) {
            pool.enqueue([&, job]() {
                job->execute();

                // Notify dependents
                auto it = _dependents.find(job.get());
                if (it != _dependents.end()) {
                    for (auto& dep : it->second) {
                        if (dep->_pending_inputs.fetch_sub(1, std::memory_order_acq_rel) == 1)
                            enqueueJob(dep);
                    }
                }

                // Last job signals completion
                if (remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    std::lock_guard<std::mutex> lock(doneMutex);
                    doneCV.notify_one();
                }
            });
        };

        // Seed jobs with no pending inputs
        for (auto& job : _jobs)
            if (job->_pending_inputs.load(std::memory_order_relaxed) == 0)
                enqueueJob(job);

        // Block until all jobs complete
        std::unique_lock<std::mutex> lock(doneMutex);
        doneCV.wait(lock, [&] { return remaining.load(std::memory_order_acquire) == 0; });

        return true;
    }

    // -------------------------------------------------------------------------
    // JobScheduler
    // -------------------------------------------------------------------------

    JobScheduler::JobScheduler(uint32_t num_threads) {
        if (num_threads == 0)
            num_threads = std::thread::hardware_concurrency();
        _pool = std::make_unique<ThreadPool>(num_threads);
    }

    JobScheduler::~JobScheduler() {
        for (auto& [graph, future] : _oneshot)
            delete future;
    }

    uint32_t JobScheduler::threadCount() const {
        return _pool->threadCount();
    }

    void JobScheduler::every_frame(const JobGraphPtr& graph) {
        _recurring.push_back(graph);
    }

    JobFuture& JobScheduler::once(const JobGraphPtr& graph) {
        auto* future = new JobFuture();
        _oneshot.push_back({ graph, future });

        // Dispatch immediately to the pool — non-blocking.
        // The future transitions Running → Done when the graph completes.
        // tick() just collects completed one-shots.
        auto* pool = _pool.get();
        future->_state.store(JobFuture::State::Running);
        pool->enqueue([graph, future, pool]() {
            auto t0 = std::chrono::high_resolution_clock::now();
            graph->execute(*pool);
            auto t1 = std::chrono::high_resolution_clock::now();

            future->_elapsed_us.store(static_cast<uint32_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count()));
            future->_state.store(JobFuture::State::Done, std::memory_order_release);
        });

        return *future;
    }

    void JobScheduler::tick() {
        // Drive recurring graphs — blocks until each completes
        for (auto& graph : _recurring)
            graph->execute(*_pool);

        // Collect completed one-shots
        _oneshot.erase(
            std::remove_if(_oneshot.begin(), _oneshot.end(),
                [](const std::pair<JobGraphPtr, JobFuture*>& entry) {
                    if (entry.second->isDone()) {
                        delete entry.second;
                        return true;
                    }
                    return false;
                }),
            _oneshot.end());
    }

    // -------------------------------------------------------------------------
    // JobGraph::executeDownstreamOf
    // -------------------------------------------------------------------------

    void JobGraph::executeDownstreamOf(const JobPtr& seedJob, ThreadPool& pool) {
        if (_dirty)
            recomputeTopoOrder();

        auto& downstream = _dependents[seedJob.get()];
        if (downstream.empty())
            return;

        // Count jobs reachable from seedJob (BFS over _dependents)
        std::vector<JobPtr> reachable;
        {
            std::vector<Job*> frontier;
            std::unordered_map<Job*, bool> visited;
            for (auto& j : downstream) {
                if (!visited[j.get()]) {
                    visited[j.get()] = true;
                    frontier.push_back(j.get());
                    reachable.push_back(j);
                }
            }
            while (!frontier.empty()) {
                Job* cur = frontier.back(); frontier.pop_back();
                auto it = _dependents.find(cur);
                if (it == _dependents.end()) continue;
                for (auto& j : it->second) {
                    if (!visited[j.get()]) {
                        visited[j.get()] = true;
                        frontier.push_back(j.get());
                        reachable.push_back(j);
                    }
                }
            }
        }

        const int total = static_cast<int>(reachable.size());

        // Reset pending counters only for reachable jobs, but count only
        // inputs that are within the reachable set OR are seedJob itself.
        std::unordered_map<Job*, bool> reachableSet;
        reachableSet[seedJob.get()] = true;
        for (auto& j : reachable) reachableSet[j.get()] = true;

        for (auto& job : reachable) {
            int pending = 0;
            for (auto& inp : job->inputJobs())
                // seedJob is already complete — don't count it as a pending input
                if (reachableSet.count(inp.get()) && inp.get() != seedJob.get()) ++pending;
            job->_pending_inputs.store(pending, std::memory_order_relaxed);
        }

        std::mutex              doneMutex;
        std::condition_variable doneCV;
        std::atomic<int>        remaining { total };

        std::function<void(const JobPtr&)> enqueueJob;
        enqueueJob = [&](const JobPtr& job) {
            pool.enqueue([&, job]() {
                job->execute();

                auto it = _dependents.find(job.get());
                if (it != _dependents.end()) {
                    for (auto& dep : it->second) {
                        if (!reachableSet.count(dep.get())) continue;
                        if (dep->_pending_inputs.fetch_sub(1, std::memory_order_acq_rel) == 1)
                            enqueueJob(dep);
                    }
                }

                if (remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    std::lock_guard<std::mutex> lock(doneMutex);
                    doneCV.notify_one();
                }
            });
        };

        // Seed: direct dependents of seedJob whose only pending input is seedJob
        for (auto& job : downstream)
            if (job->_pending_inputs.load(std::memory_order_relaxed) == 0)
                enqueueJob(job);

        std::unique_lock<std::mutex> lock(doneMutex);
        doneCV.wait(lock, [&] { return remaining.load(std::memory_order_acquire) == 0; });
    }

    // -------------------------------------------------------------------------
    // EventJobHandle
    // -------------------------------------------------------------------------

    EventJobHandle::~EventJobHandle() {
        _running.store(false);
        if (_thread.joinable())
            _thread.join();
    }

    void EventJobHandle::_loop() {
        setThreadName("pico::Job::" + _job->name());
        while (_running.load()) {
            // Block on the event (vsync etc.) — no mutex held during wait
            bool fired = _waitFn ? _waitFn(100) : true;
            if (!_running.load()) break;
            if (!fired) continue;

            // No-op kernel; marks Trigger output ready (release semantics)
            _job->execute();

            // Hold frameMutex while dispatching downstream and waiting for completion
            {
                std::lock_guard<std::mutex> lock(_frameMutex);
                _job->graph()->executeDownstreamOf(_job, *_pool);
            }
            // Mutex released here — window for resize or other external ops
        }
    }

    EventJobHandlePtr JobScheduler::createEventJob(std::string_view name, WaitFn waitFn) {
        auto handle = std::shared_ptr<EventJobHandle>(new EventJobHandle());
        handle->_pool    = _pool.get();
        handle->_waitFn  = std::move(waitFn);

        auto job = Job::create(name);
        auto trigger_out = job->output<Trigger>("trigger");
        // No kernel — _loop drives the wait directly; execute() just marks slots ready
        handle->output = trigger_out;
        handle->_job   = job;

        handle->_running.store(true);
        handle->_thread = std::thread([h = handle.get()] { h->_loop(); });

        return handle;
    }

} // namespace core

// -------------------------------------------------------------------------
// Simple test — call runJobTests() to validate the job graph wiring
// -------------------------------------------------------------------------

void runJobTests() {
    using namespace core;

    picoLog("JobTest: starting...");

    // --- Test 1: single job, one output ---
    {
        auto producer  = Job::create("producer");
        auto value_out = producer->output<int>("value");

        producer->kernel([](int& value) {
            value = 42;
        });

        JobScheduler scheduler;
        scheduler.every_frame(producer->graph());
        scheduler.tick();

        assert(value_out._job->accessOutput<int>(value_out._slot_index) == 42);
        picoLog("JobTest 1 passed: single job produces value");
    }

    // --- Test 2: two jobs, output wired to input ---
    {
        auto job_a = Job::create("job_a");
        auto job_b = Job::create("job_b");

        auto a_out = job_a->output<int>("a_value");
        auto b_out = job_b->output<int>("b_value");

        job_a->kernel([](int& out) { out = 7; });

        job_b->input(a_out)
              .kernel([](const int& a, int& b) { b = a * 2; });  // expects 14

        assert(job_b->graph() == job_a->graph());

        JobScheduler scheduler;
        scheduler.every_frame(job_b->graph());
        scheduler.tick();

        assert(job_b->accessOutput<int>(b_out._slot_index) == 14);
        picoLog("JobTest 2 passed: two chained jobs, result correct");
    }

    // --- Test 3: diamond dependency ---
    {
        // source → left  ─┐
        //        → right ─┴→ sink
        auto source = Job::create("source");
        auto left   = Job::create("left");
        auto right  = Job::create("right");
        auto sink   = Job::create("sink");

        auto src_out   = source->output<int>("src");
        auto left_out  = left->output<int>("left");
        auto right_out = right->output<int>("right");
        auto sink_out  = sink->output<int>("sink");

        source->kernel([](int& v)                          { v = 10; });
        left  ->input(src_out)
               .kernel([](const int& s, int& l)            { l = s + 1; });   // 11
        right ->input(src_out)
               .kernel([](const int& s, int& r)            { r = s + 2; });   // 12
        sink  ->input(left_out)
               .input(right_out)
               .kernel([](const int& l, const int& r, int& out) { out = l + r; }); // 23

        JobScheduler scheduler;
        scheduler.every_frame(sink->graph());
        scheduler.tick();

        assert(sink->accessOutput<int>(sink_out._slot_index) == 23);
        picoLog("JobTest 3 passed: diamond dependency, result correct");
    }

    // --- Test 4: scheduler recurring graph ---
    {
        auto job       = Job::create("counter");
        auto count_out = job->output<int>("count");

        int tick_count = 0;
        job->kernel([&tick_count](int& count) { count = ++tick_count; });

        JobScheduler scheduler;
        scheduler.every_frame(job->graph());

        scheduler.tick();
        scheduler.tick();
        scheduler.tick();

        assert(tick_count == 3);
        assert(job->accessOutput<int>(count_out._slot_index) == 3);
        picoLog("JobTest 4 passed: scheduler recurring graph ticked 3 times");
    }

    // --- Test 5: parallel jobs run concurrently ---
    {
        // Two independent jobs — should run on separate threads
        auto job_a = Job::create("parallel_a");
        auto job_b = Job::create("parallel_b");
        auto sink  = Job::create("parallel_sink");

        auto a_out = job_a->output<int>("a");
        auto b_out = job_b->output<int>("b");
        auto s_out = sink->output<int>("sum");

        std::atomic<int> concurrency { 0 };
        std::atomic<int> max_concurrency { 0 };

        auto track = [&]() {
            int c = concurrency.fetch_add(1, std::memory_order_relaxed) + 1;
            int m = max_concurrency.load(std::memory_order_relaxed);
            while (c > m && !max_concurrency.compare_exchange_weak(m, c)) {}
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            concurrency.fetch_sub(1, std::memory_order_relaxed);
        };

        job_a->kernel([&track](int& a) { track(); a = 3; });
        job_b->kernel([&track](int& b) { track(); b = 4; });
        sink ->input(a_out)
              .input(b_out)
              .kernel([](const int& a, const int& b, int& sum) { sum = a + b; });

        JobScheduler scheduler;
        scheduler.every_frame(sink->graph());
        scheduler.tick();

        assert(sink->accessOutput<int>(s_out._slot_index) == 7);
        // With >= 2 threads, parallel jobs should have run concurrently
        if (scheduler.threadCount() >= 2)
            assert(max_concurrency.load() >= 2);
        picoLog("JobTest 5 passed: parallel jobs, result correct");
    }

    // --- Test 6: async load → process chain via once() ---
    {
        struct AssetData      { std::string raw;       bool loaded   { false }; };
        struct ProcessedAsset { std::string processed; bool ready    { false }; };

        auto load    = Job::create("load_asset");
        auto process = Job::create("process_asset");

        auto asset_out    = load->output<AssetData>     ("asset");
        auto processed_out = process->output<ProcessedAsset>("processed");

        // Simulate a slow disk read
        load->kernel([](AssetData& asset) {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            asset.raw    = "raw_data";
            asset.loaded = true;
        });

        // Runs on whatever thread is free after load — no constraint
        process->input(asset_out)
                .kernel([](const AssetData& asset, ProcessedAsset& result) {
                    assert(asset.loaded);
                    result.processed = asset.raw + "_processed";
                    result.ready     = true;
                });

        JobScheduler scheduler;
        JobFuture& future = scheduler.once(process->graph());

        // Simulate a few frames polling for completion
        int frames = 0;
        while (!future.isDone()) {
            scheduler.tick();   // drives recurring graphs (none here), collects done one-shots
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ++frames;
            assert(frames < 100 && "load timed out");
        }

        const auto& result = process->accessOutput<ProcessedAsset>(processed_out._slot_index);
        assert(result.ready);
        assert(result.processed == "raw_data_processed");
        picoLogf("JobTest 6 passed: async load→process chain done in {} frames, {}us",
            frames, future.elapsedUs());
    }

    picoLog("JobTest: all tests passed.");
}
