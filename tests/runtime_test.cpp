#include <arbiter0/runtime.hpp>
#include <cassert>

int main() {
    arbiter0::Runtime runtime;
    std::vector<int> events;

    runtime.spawn([&] {
        events.push_back(10);
    });

    runtime.spawn([&] {
        events.push_back(20);
    });

    runtime.run({1, 0});

    assert((events == std::vector<int>{20, 10}));
}