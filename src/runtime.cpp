#include <arbiter0/runtime.hpp>

namespace arbiter0 {

ThreadId Runtime::spawn(std::function<void()> task) {
    ThreadId id = tasks_.size();
    tasks_.push_back(std::move(task));
    return id; // sequential id
}

void Runtime::run() {
    for (auto& task: tasks_) {
        task();
    }
}

}