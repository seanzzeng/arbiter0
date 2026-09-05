#pragma once

#include <cstddef>
#include <functional>
#include <vector>
#include <mutex>
#include <condition_variable>

namespace arbiter0 {

using ThreadId = std::size_t;

class Runtime {
public:
    ThreadId spawn(std::function<void()> task);
    void run(const std::vector<ThreadId>& schedule);

private:
    enum class WorkerState {
        created,
        runnable,
        running,
        finished
    };

    std::vector<std::function<void()>> tasks_;
    std::vector<WorkerState> states_;

    std::mutex mutex_;
    std::condition_variable cv_;

    std::size_t ready_cnt_ = 0;

};

}