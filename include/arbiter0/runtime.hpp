#pragma once

#include <cstddef>
#include <functional>
#include <vector>

namespace arbiter0 {

using ThreadId = std::size_t;

class Runtime {
public:
    ThreadId spawn(std::function<void()> task);
    void run();

private:
    std::vector<std::function<void()>> tasks_;
};

}