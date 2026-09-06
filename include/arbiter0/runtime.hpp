#pragma once

#include <cstddef>
#include <functional>
#include <vector>
#include <mutex>
#include <condition_variable>

namespace arbiter0 {

using ThreadId = std::size_t;

class Runtime;

class ThreadContext {
public:
    void yield();

private:
    friend class Runtime;

    ThreadContext(Runtime& runtime, ThreadId id): runtime_(runtime), id_(id) {}
    
    Runtime& runtime_;
    ThreadId id_;
};

class Runtime {
public:
    ThreadId spawn(std::function<void(ThreadContext&)> task);
    void run(const std::vector<ThreadId>& schedule);

private:
    friend class ThreadContext;
    void yield(ThreadId id);

    enum class WorkerState {
        created,
        runnable,
        running,
        finished
    };

    std::vector<std::function<void(ThreadContext&)>> tasks_;
    std::vector<WorkerState> states_;

    std::mutex mutex_;
    std::condition_variable cv_;

    std::size_t ready_cnt_ = 0;

};

}