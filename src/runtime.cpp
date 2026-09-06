#include <arbiter0/runtime.hpp>
#include <utility>
#include <vector>
#include <thread>
#include <stdexcept>
#include <cassert>

namespace arbiter0 {

ThreadId Runtime::spawn(std::function<void(ThreadContext&)> task) {
    ThreadId id = tasks_.size();
    tasks_.push_back(std::move(task));
    return id; // sequential id
}

void Runtime::run(const std::vector<ThreadId>& schedule) {
    for (ThreadId id: schedule) {
        if (id >= tasks_.size()) {
            throw std::invalid_argument {
                "schedule contains invalid ID"
            };
        }
    }
    states_.assign(tasks_.size(), WorkerState::created);

    ready_cnt_ = 0;
    stopping_ = false;

    std::vector<std::thread> run_threads; 
    run_threads.reserve(tasks_.size());
    for (ThreadId id = 0; id < tasks_.size(); ++id) {
        run_threads.emplace_back([this, id] {
            try {
                {
                    std::unique_lock lock(mutex_);

                    states_[id] = WorkerState::runnable;

                    ++ready_cnt_;
                    cv_.notify_all();

                    cv_.wait(lock, [this, id] {
                        return states_[id] == WorkerState::running;
                    });
                }

                // *this is runtime obj itself
                ThreadContext context(*this, id);
                tasks_[id](context);
            } catch (const RunCancelled&) {
                // cleanup
            }

            {
                std::lock_guard lock(mutex_);
                states_[id] = WorkerState::finished;
            }

            cv_.notify_all();
        
        });
    }

    const char* schedule_err = nullptr;

    {
        std::unique_lock lock(mutex_);

        cv_.wait(lock, [this] {
            return ready_cnt_ == tasks_.size();
        });

        for (auto& id: schedule) {
            if (states_[id] != WorkerState::runnable) {
                schedule_err = "schedule selects finished worker";
                break;
            }

            states_[id] = WorkerState::running;
            cv_.notify_all();

            // either yielded or finished
            cv_.wait(lock, [this, id] {
                return states_[id] != WorkerState::running;
            });
        }

        if (schedule_err == nullptr) {
            for (WorkerState state: states_) {
                if (state != WorkerState::finished) {
                    schedule_err = "schedule ended before all workers finished";
                    break;
                }
            }
        }

        if (schedule_err != nullptr) {
            stopping_ = true;
            cv_.notify_all();
        }
    }

    // cleanup
    for (auto& worker: run_threads) {
        worker.join();
    }

    if (schedule_err != nullptr) {
        throw std::invalid_argument(schedule_err);
    }
}

void ThreadContext::yield() {
    runtime_.yield(id_);
}

void Runtime::yield(ThreadId id) {
    {
        std::unique_lock lock(mutex_);
        assert(states_[id] == WorkerState::running);

        states_[id] = WorkerState::runnable;

        cv_.notify_all();
        
        cv_.wait(lock, [this, id] {
            return stopping_ || states_[id] == WorkerState::running;
        });

        if (stopping_) {
            throw RunCancelled{};
        }
    }
}

}