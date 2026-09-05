#include <arbiter0/runtime.hpp>
#include <cassert>
#include <vector>
#include <stdexcept>

int main() {
    arbiter0::Runtime runtime;
    std::vector<int> events;

    runtime.spawn([&](arbiter0::ThreadContext&) {
        events.push_back(10);
    });

    runtime.spawn([&](arbiter0::ThreadContext&) {
        events.push_back(20);
    });

    runtime.run({1, 0});

    assert((events == std::vector<int>{20, 10}));
    
    bool rejected = false;
    try {
        runtime.run({0, 0});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    assert(rejected);

    events.clear();
    runtime.run({0, 1});
    assert((events == std::vector<int>{10, 20}));
}