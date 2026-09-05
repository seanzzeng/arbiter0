cmake_minimum_required(VERSION 3.20)

project(arbiter0 VERSION 0.1.0 LANGUAGES CXX)

add_library(arbiter0
    src/runtime.cpp
)

target_include_directories(arbiter0 PUBLIC
    include
)

target_compile_features(arbiter0 PUBLIC
    cxx_std_20
)

add_executable(runtime_test
    tests/runtime_test.cpp
)

target_link_libraries(runtime_test PRIVATE
    arbiter0
)

find_package(Threads REQUIRED)

target_link_libraries(arbiter0 PRIVATE
    Threads::Threads
)

enable_testing()

add_test(
    NAME runtime_test
    COMMAND runtime_test
)