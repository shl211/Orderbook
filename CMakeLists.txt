cmake_minimum_required(VERSION 3.30)

project(
    orderbook-lib
    VERSION 0.1
    DESCRIPTION "Orderbook implementations"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)

add_library(orderbook-lib INTERFACE)

target_include_directories(orderbook-lib
    INTERFACE
        ${PROJECT_SOURCE_DIR}/include
)

option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    enable_testing()
    
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
    )
    FetchContent_MakeAvailable(googletest)

    add_subdirectory(tests)
endif()

option(BUILD_BENCH "Build benchmarks" OFF)

if(BUILD_BENCH)
    include(FetchContent)

    FetchContent_Declare(
        cxxopts
        GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
        GIT_TAG v3.1.1
    )
    FetchContent_MakeAvailable(cxxopts)

    add_subdirectory(benchmark)
endif()