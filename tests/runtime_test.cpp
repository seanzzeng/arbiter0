#include <arbiter0/runtime.hpp>
#include <cassert>
#include <vector>
#include <stdexcept>
#include <string>

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

    rejected = false;

    // not finished
    try {
        runtime.run({0, 1});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    assert(rejected);

    rejected = false;

    // cancelling partial + full task
    try {
        runtime.run({0});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    assert(rejected);

    rejected = false;

    // reject finished worker
    try {
        runtime.run({0, 0, 0});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    assert(rejected);

    events.clear();
    runtime.run({0, 0, 1, 1});
    assert((events == std::vector<int>{10, 11, 20, 21}));

    // test exception is thrown on the thread calling run()
    {
        arbiter0::Runtime failing_runtime;
        bool second_ran = false;
        bool caught = false;

        failing_runtime.spawn([](arbiter0::ThreadContext&) {
            throw std::runtime_error("a runtime error has occurred");
        });

        failing_runtime.spawn([&](arbiter0::ThreadContext&) {
            second_ran = true;
        });

        try {
            failing_runtime.run({0, 1});
        } catch (const std::runtime_error& err) {
            caught = true;
            assert(std::string(err.what()) == "a runtime error has occurred");
        }

        assert(caught);
        assert(!second_ran);
    }
}