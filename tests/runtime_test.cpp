#include <arbiter0/runtime.hpp>
#include <cassert>
#include <vector>

int main() {
    arbiter0::Runtime runtime;
    std::vector<int> events;

    int first_result = 0;
    int second_result = 0;

    runtime.spawn([&] {
        first_result = 10;
    });

    runtime.spawn([&] {
        second_result = 20;
    });

    runtime.run();

    assert(first_result == 10);
    assert(second_result == 20);
}