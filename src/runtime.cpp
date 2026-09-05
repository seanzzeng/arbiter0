#include <arbiter0/runtime.hpp>
#include <utility>
#include <vector>
#include <thread>

namespace arbiter0 {

ThreadId Runtime::spawn(std::function<void()> task) {
    ThreadId id = tasks_.size();
    tasks_.push_back(std::move(task));
    return id; // sequential id
}

void Runtime::run() {
    std::vector<std::thread> run_threads; 
    run_threads.reserve(tasks_.size());
    for (auto& task: tasks_) {
        run_threads.emplace_back(task);
    }

    for (auto& worker: run_threads) {
        worker.join();
    }
}

}