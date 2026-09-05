#include <arbiter0/runtime.hpp>
#include <cassert>

int main() {
    arbiter0::Runtime runtime;
    std::vector<int> events;

    auto first = runtime.spawn([&] {
        events.push_back(10);
    });

    auto second = runtime.spawn([&] {
        events.push_back(20);
    });

    runtime.run();

    assert(first == 0);
    assert(second == 1);
    assert(events.size() == 2);
    assert(events[0] == 10);
    assert(events[1] == 20);
}