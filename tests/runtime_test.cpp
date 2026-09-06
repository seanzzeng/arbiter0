#include <arbiter0/runtime.hpp>
#include <cassert>
#include <vector>
#include <stdexcept>

int main() {
    arbiter0::Runtime runtime;
    std::vector<int> events;

    runtime.spawn([&](arbiter0::ThreadContext& ctx) {
        events.push_back(10);
        ctx.yield();
        events.push_back(11);
    });

    runtime.spawn([&](arbiter0::ThreadContext& ctx) {
        events.push_back(20);
        ctx.yield();
        events.push_back(21);
    });

    runtime.run({0, 1, 0, 1});

    assert((events == std::vector<int>{10, 20, 11, 21}));
    
    bool rejected = false;
    try {
        runtime.run({5, 0});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    assert(rejected);

    events.clear();
    runtime.run({0, 0, 1, 1});
    assert((events == std::vector<int>{10, 11, 20, 21}));
}