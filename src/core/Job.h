// Job.h
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
#pragma once

#include <memory>
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <tuple>
#include <type_traits>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <unordered_map>
#include <cstdint>
#include <cassert>

#include "dllmain.h"

namespace core {

    // -------------------------------------------------------------------------
    // Forward declarations
    // -------------------------------------------------------------------------

    class Job;
    class JobGraph;

    using JobPtr      = std::shared_ptr<Job>;
    using JobGraphPtr = std::shared_ptr<JobGraph>;

    // -------------------------------------------------------------------------
    // ThreadPool
    //
    // Fixed-size pool of worker threads pulling from a shared work queue.
    // Constructed once by JobScheduler (defaults to hardware_concurrency).
    // -------------------------------------------------------------------------

    class CORE_API ThreadPool {
    public:
        explicit ThreadPool(uint32_t num_threads);
        ~ThreadPool();

        void     enqueue(std::function<void()> fn);
        uint32_t threadCount() const { return static_cast<uint32_t>(_workers.size()); }

    private:
        std::vector<std::thread>          _workers;
        std::deque<std::function<void()>> _queue;
        std::mutex                        _mutex;
        std::condition_variable           _cv;
        bool                              _stop { false };
    };

    // -------------------------------------------------------------------------
    // JobSlotBase / JobSlot<T>
    //
    // A slot is a typed value owned by a Job.
    // Allocated once at graph build time, overwritten each frame by the kernel.
    // Reset policy: explicit — the kernel is responsible for clearing the value.
    //
    // _ready is atomic with release/acquire semantics:
    //   - producing job stores true  (release) after writing _value
    //   - consuming job loads  true  (acquire) before reading _value
    //   This guarantees _value visibility across threads without extra locks.
    // -------------------------------------------------------------------------

    struct JobSlotBase {
        std::string       _name;
        std::atomic<bool> _ready { false };

        explicit JobSlotBase(std::string_view name) : _name(name) {}
        virtual ~JobSlotBase() = default;

        JobSlotBase(const JobSlotBase&)            = delete;
        JobSlotBase& operator=(const JobSlotBase&) = delete;
    };

    template<typename T>
    struct JobSlot : JobSlotBase {
        T _value {};

        explicit JobSlot(std::string_view name) : JobSlotBase(name) {}
    };

    // -------------------------------------------------------------------------
    // JobOutput<T>
    //
    // Typed handle to one output slot of one Job.
    //   - Keeps the producing Job alive via shared_ptr
    //   - Passed to another job's .input() to declare a dependency edge
    //     (triggers graph merge)
    //   - _job + _slot_index together identify the exact slot for data access
    // -------------------------------------------------------------------------

    template<typename T>
    struct JobOutput {
        JobPtr   _job;
        uint32_t _slot_index { 0 };

        JobOutput() = default;
        JobOutput(JobPtr job, uint32_t slot_index)
            : _job(std::move(job)), _slot_index(slot_index) {}

        bool valid() const { return _job != nullptr; }
    };

    // -------------------------------------------------------------------------
    // function_traits — extract argument types from a callable
    // -------------------------------------------------------------------------

    template<typename Fn>
    struct function_traits : function_traits<decltype(&Fn::operator())> {};

    template<typename Ret, typename... Args>
    struct function_traits<Ret(*)(Args...)> {
        using args = std::tuple<Args...>;
        static constexpr size_t arity = sizeof...(Args);
    };

    template<typename Class, typename Ret, typename... Args>
    struct function_traits<Ret(Class::*)(Args...) const> {
        using args = std::tuple<Args...>;
        static constexpr size_t arity = sizeof...(Args);
    };

    template<typename Class, typename Ret, typename... Args>
    struct function_traits<Ret(Class::*)(Args...)> {
        using args = std::tuple<Args...>;
        static constexpr size_t arity = sizeof...(Args);
    };

    // -------------------------------------------------------------------------
    // SlotResolver — type-erased accessor to a slot value
    // -------------------------------------------------------------------------

    struct SlotResolver {
        std::function<void*()> _resolve;
        void* get() const { return _resolve(); }
    };

    // -------------------------------------------------------------------------
    // JobGraph
    //
    // Never constructed directly by the user.
    // Created implicitly when a Job is constructed.
    // Merged with another graph when job_b.input(job_a_output) is called.
    //
    // Multithreaded execution (Kahn's algorithm driven by completion):
    //   - Seed thread pool with jobs whose _pending_inputs == 0
    //   - On job completion: for each dependent, decrement _pending_inputs
    //     atomically — if it hits 0, enqueue to thread pool
    //   - execute() blocks until all jobs complete via a condition variable
    // -------------------------------------------------------------------------

    class CORE_API JobGraph : public std::enable_shared_from_this<JobGraph> {
    public:
        JobGraph() = default;

        void addJob(const JobPtr& job);
        void merge(const JobGraphPtr& other);

        // Execute all jobs using the thread pool. Blocks until all complete.
        // Returns false if a cycle is detected.
        bool execute(ThreadPool& pool);

        // Execute all jobs sequentially in topo order on the calling thread.
        void executeOnCurrentThread();

        // Execute all jobs downstream of seedJob using the pool.
        // seedJob must already have been executed by the caller.
        // Blocks until all downstream jobs complete.
        void executeDownstreamOf(const JobPtr& seedJob, ThreadPool& pool);

        const std::vector<JobPtr>& jobs() const { return _jobs; }

    private:
        friend class Job;

        std::vector<JobPtr>                           _jobs;
        bool                                          _dirty { true };
        std::vector<JobPtr>                           _topoOrder;
        std::unordered_map<Job*, std::vector<JobPtr>> _dependents;  // producer → consumers

        void recomputeTopoOrder();
    };

    // -------------------------------------------------------------------------
    // Job
    //
    // Named unit of work with typed input/output slot declarations.
    // Kernel signature: inputs first (const T&), then outputs (T&),
    // in declaration order.
    //
    // Example:
    //
    //   auto cull        = Job::create("cull_camera");
    //   auto visible_ids = cull->output<ItemIDs>("visible_item_ids");
    //
    //   cull->input(updated_nodes)
    //        .kernel([](const NodeStore* nodes, ItemIDs& result) {
    //            result.clear();
    //            // fill result from nodes...
    //        });
    // -------------------------------------------------------------------------

    class CORE_API Job : public std::enable_shared_from_this<Job> {
    public:
        static JobPtr create(std::string_view name);

        const std::string& name() const { return _name; }

        // Declare an output slot — returns a typed handle for wiring and access
        template<typename T>
        JobOutput<T> output(std::string_view slot_name) {
            uint32_t idx = static_cast<uint32_t>(_output_slots.size());
            _output_slots.push_back(std::make_unique<JobSlot<T>>(slot_name));

            Job* self = this;
            _write_resolvers.push_back(SlotResolver {
                [self, idx]() -> void* {
                    return &static_cast<JobSlot<T>*>(self->_output_slots[idx].get())->_value;
                }
            });

            return JobOutput<T>{ shared_from_this(), idx };
        }

        // Declare an input dependency — merges graphs, registers resolver.
        // _job in 'out' tracks the producing job (for topo sort).
        // _slot_index in 'out' is captured in the resolver (for data access).
        template<typename T>
        Job& input(const JobOutput<T>& out) {
            assert(out.valid());

            _read_resolvers.push_back(SlotResolver {
                [out]() -> void* {
                    assert(out._job->_output_slots[out._slot_index]->_ready.load(std::memory_order_acquire));
                    return &out._job->accessOutput<T>(out._slot_index);
                }
            });

            _input_jobs.push_back(out._job);
            _graph->merge(out._job->_graph);

            return *this;
        }

        // Bind the kernel — parameter count must equal input() + output() calls.
        // Parameters: inputs first (const T&), then outputs (T&), in declaration order.
        template<typename KernelFn>
        Job& kernel(KernelFn&& fn) {
            using traits = function_traits<std::decay_t<KernelFn>>;
            constexpr size_t arity = traits::arity;

            assert(arity == _read_resolvers.size() + _write_resolvers.size()
                && "kernel parameter count must match inputs + outputs declarations");

            std::vector<SlotResolver> resolvers;
            resolvers.insert(resolvers.end(), _read_resolvers.begin(),  _read_resolvers.end());
            resolvers.insert(resolvers.end(), _write_resolvers.begin(), _write_resolvers.end());

            _kernel = bindKernel(std::forward<KernelFn>(fn),
                                 std::move(resolvers),
                                 std::make_index_sequence<arity>{});
            return *this;
        }

        // Called by JobGraph — marks slots not-ready, runs kernel, marks ready (release)
        void execute();

        const std::vector<JobPtr>&                       inputJobs()   const { return _input_jobs; }
        const std::vector<std::unique_ptr<JobSlotBase>>& outputSlots() const { return _output_slots; }
        JobGraphPtr                                      graph()       const { return _graph; }

        template<typename T>
        T& accessOutput(uint32_t slot_index) {
            assert(slot_index < _output_slots.size());
            return static_cast<JobSlot<T>*>(_output_slots[slot_index].get())->_value;
        }

        template<typename T>
        const T& accessOutput(uint32_t slot_index) const {
            assert(slot_index < _output_slots.size());
            return static_cast<const JobSlot<T>*>(_output_slots[slot_index].get())->_value;
        }

    private:

        template<typename KernelFn, size_t... Is>
        std::function<void()> bindKernel(KernelFn&& fn,
                                          std::vector<SlotResolver> resolvers,
                                          std::index_sequence<Is...>) {
            using traits    = function_traits<std::decay_t<KernelFn>>;
            using ArgsTuple = typename traits::args;

            return [fn = std::forward<KernelFn>(fn), resolvers = std::move(resolvers)]() mutable {
                fn(*static_cast<std::remove_reference_t<std::tuple_element_t<Is, ArgsTuple>>*>(
                    resolvers[Is].get())...);
            };
        }

        explicit Job(std::string_view name);   // use Job::create()

        friend class JobGraph;

        std::string                                  _name;
        JobGraphPtr                                  _graph;
        std::vector<std::unique_ptr<JobSlotBase>>    _output_slots;
        std::vector<JobPtr>                          _input_jobs;       // for topo sort
        std::vector<SlotResolver>                    _read_resolvers;   // one per .input() call
        std::vector<SlotResolver>                    _write_resolvers;  // one per .output() call
        std::function<void()>                        _kernel;

        // Runtime dispatch state — reset at the start of each graph execute()
        std::atomic<int>                             _pending_inputs { 0 };
    };

    // -------------------------------------------------------------------------
    // JobFuture — observable state for one-shot graph execution
    // -------------------------------------------------------------------------

    struct CORE_API JobFuture {
        enum class State { Pending, Running, Done };

        State    state()     const { return _state.load(); }
        bool     isDone()    const { return _state.load() == State::Done; }
        uint32_t elapsedUs() const { return _elapsed_us.load(); }

    private:
        friend class JobScheduler;
        std::atomic<State>    _state      { State::Pending };
        std::atomic<uint32_t> _elapsed_us { 0 };
    };

    // WaitFn: platform-agnostic event wait. Returns true if the event fired,
    // false on timeout. Used by EventJob to block on e.g. vsync.
    using WaitFn = std::function<bool(uint32_t timeoutMs)>;

    // Empty signal type — EventJob output that downstream jobs depend on.
    struct Trigger {};

    // -------------------------------------------------------------------------
    // EventJobHandle
    //
    // Returned by JobScheduler::createEventJob(). Owns the dedicated persistent
    // thread that blocks on the WaitFn each frame.
    //
    // Frame lifecycle on the dedicated thread:
    //   1. EventJob kernel runs — blocks on WaitFn until event fires
    //   2. frameMutex acquired
    //   3. All downstream jobs dispatched to the scheduler's thread pool
    //   4. Wait for all downstream jobs to complete
    //   5. frameMutex released  ← window for external code (e.g. resize)
    //   6. Go to 1
    //
    // Wire downstream jobs:
    //   presentJob->input(vsync->output).kernel([](const Trigger&) { ... });
    //
    // Synchronize with resize or other external operations:
    //   std::lock_guard lock(vsync->frameMutex());
    //   device->resizeSwapchain(...);
    // -------------------------------------------------------------------------

    class CORE_API EventJobHandle {
    public:
        ~EventJobHandle();

        JobOutput<Trigger>  output;
        std::mutex&         frameMutex() { return _frameMutex; }

    private:
        friend class JobScheduler;
        EventJobHandle() = default;

        WaitFn              _waitFn;
        JobPtr              _job;
        std::mutex          _frameMutex;
        std::atomic<bool>   _running { false };
        std::thread         _thread;
        ThreadPool*         _pool { nullptr };

        void _loop();
    };

    using EventJobHandlePtr = std::shared_ptr<EventJobHandle>;

    // -------------------------------------------------------------------------
    // JobScheduler — drives JobGraphs, recurring or one-shot
    //
    //   scheduler.every_frame(graph);          // re-executed each tick()
    //   auto& future = scheduler.once(graph);  // executed once, observable
    //   scheduler.tick();                      // call once per frame, blocks until done
    //   auto vsync = scheduler.createEventJob("vsync", waitFn);
    // -------------------------------------------------------------------------

    class CORE_API JobScheduler {
    public:
        // num_threads == 0 defaults to hardware_concurrency
        explicit JobScheduler(uint32_t num_threads = 0);
        ~JobScheduler();

        void       every_frame(const JobGraphPtr& graph);
        JobFuture& once(const JobGraphPtr& graph);
        void       tick();

        EventJobHandlePtr createEventJob(std::string_view name, WaitFn waitFn);

        uint32_t threadCount() const;

    private:
        std::unique_ptr<ThreadPool>                      _pool;
        std::vector<JobGraphPtr>                         _recurring;
        std::vector<std::pair<JobGraphPtr, JobFuture*>>  _oneshot;
    };

} // namespace core
