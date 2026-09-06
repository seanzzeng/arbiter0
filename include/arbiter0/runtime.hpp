#pragma once

#include <cstddef>
#include <functional>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <exception>

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
    // cancellation signal
    struct RunCancelled {};

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
    bool stopping_ = false;

    // exceptions unwind the thread they were thrown on, pass back to caller safely
    std::exception_ptr worker_err_;

};

}