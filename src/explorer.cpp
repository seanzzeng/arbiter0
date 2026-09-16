#include <arbiter0/explorer.hpp>

#include <stdexcept>
#include <utility>

namespace arbiter0 {

ExplorationResult explore(
    const ExecuteFresh& execute,
    std::size_t max_turns
) {
    ExplorationResult result;

    std::vector<std::vector<ThreadId>> pending;
    // empty vector
    pending.emplace_back();

    // 
    while (!pending.empty()) {
        auto prefix = std::move(pending.back());
        pending.pop_back();
        ExecutionResult execution = execute(prefix);

        if (execution.status == ExecutionStatus::failed) {
            result.failures.push_back(std::move(prefix));
            continue;
        }

        if (execution.status == ExecutionStatus::completed) {
            ++result.completed;
            continue;
        }

        if (execution.runnable.empty()) {
            throw std::logic_error("unfinished execution has no runnable workers");
        }

        if (prefix.size() == max_turns) {
            // reached specified execution step limit
            ++result.bounded;
            continue;
        }

        for (ThreadId id: execution.runnable) {
            auto child = prefix;
            child.push_back(id);
            pending.push_back(std::move(child));
        }
    }

    return result;
}

}