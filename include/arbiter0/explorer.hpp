#pragma once

#include <arbiter0/runtime.hpp>

#include <cstddef>
#include <functional>
#include <vector>

namespace arbiter0 {

// each call creates fresh scenario state, executes prefix    
using ExecuteFresh = std::function<ExecutionResult(const std::vector<ThreadId>&)>;

struct ExplorationResult {
    std::vector<std::vector<ThreadId>> failures;
    std::size_t completed = 0;
    std::size_t bounded = 0;
};

ExplorationResult explore(
    const ExecuteFresh& execute,
    std::size_t max_turns
);

}