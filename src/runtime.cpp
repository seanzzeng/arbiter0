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

const std::vector<TraceStep>& Runtime::trace() const {
    return trace_;
}

ExecutionResult Runtime::run_prefix(const std::vector<ThreadId>& schedule) {
    trace_.clear();

    for (ThreadId id: schedule) {
        if (id >= tasks_.size()) {
            throw std::invalid_argument {
                "schedule contains invalid ID"
            };
        }
    }

    trace_.reserve(schedule.size());
    states_.assign(tasks_.size(), WorkerState::created);

    ready_cnt_ = 0;
    stopping_ = false;
    worker_err_ = nullptr;

    std::vector<std::thread> run_threads; 
    run_threads.reserve(tasks_.size());

    ExecutionResult result{
        ExecutionStatus::completed,
        {},
        nullptr
    };

    result.runnable.reserve(tasks_.size());

    try {
        for (ThreadId id = 0; id < tasks_.size(); ++id) {
            run_threads.emplace_back([this, id] {
                try {
                    {
                        std::unique_lock lock(mutex_);

                        states_[id] = WorkerState::runnable;

                        ++ready_cnt_;
                        cv_.notify_all();

                        cv_.wait(lock, [this, id] {
                            return stopping_ || states_[id] == WorkerState::running;
                        });

                        if (stopping_) {
                            throw RunCancelled{};
                        }
                    }

                    // *this is runtime obj itself
                    ThreadContext context(*this, id);
                    tasks_[id](context);
                } catch (const RunCancelled&) {
                    // cleanup (expected cancellation)
                } catch (...) {
                    std::lock_guard lock(mutex_);

                    if (!worker_err_) {
                        worker_err_ = std::current_exception();
                    }
                }

                {
                    std::lock_guard lock(mutex_);
                    states_[id] = WorkerState::finished;
                }

                cv_.notify_all();
            
            });
        }
    } catch (...) {
        // startup failure
        {
            std::lock_guard lock(mutex_);
            stopping_ = true;
        }

        cv_.notify_all();

        for (auto &worker: run_threads) {
            worker.join();
        }

        throw;
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

            if (worker_err_) {
                trace_.emplace_back(id, StepOutcome::failed);
            } else if (states_[id] == WorkerState::runnable) {
                trace_.emplace_back(id, StepOutcome::yielded);
            } else {
                assert(states_[id] == WorkerState::finished);
                trace_.emplace_back(id, StepOutcome::finished);
            }

            if (worker_err_) {
                break;
            }
        }

        // get snapshot
        if (schedule_err == nullptr) {
            if (worker_err_) {
                result.status = ExecutionStatus::failed;
                result.error = worker_err_;
            } else {
                for (ThreadId id = 0; id < states_.size(); ++id) {
                    if (states_[id] == WorkerState::runnable) {
                        result.runnable.push_back(id);
                    } else {
                        assert(states_[id] == WorkerState::finished);
                    }
                }

                result.status = result.runnable.empty() ? ExecutionStatus::completed : ExecutionStatus::needs_choice;
            }
        }
        stopping_ = true;
        cv_.notify_all();
    }

    // cleanup
    for (auto& worker: run_threads) {
        worker.join();
    }

    if (schedule_err != nullptr) {
        throw std::invalid_argument(schedule_err);
    }

    return result;
}

void Runtime::run(const std::vector<ThreadId>& schedule) {
    ExecutionResult result = run_prefix(schedule);

    if (result.status == ExecutionStatus::failed) {
        std::rethrow_exception(result.error);
    }

    if (result.status == ExecutionStatus::needs_choice) {
        throw std::invalid_argument("schedule ended before all workers finished");
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