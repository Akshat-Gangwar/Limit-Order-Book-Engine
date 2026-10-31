cmake_minimum_required(VERSION 3.20)
project(LowLatencyLOB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

include_directories(include)

option(LOB_PARANOID "Check the full PriceLevel volume invariant after mutations" OFF)
if(LOB_PARANOID)
    add_compile_definitions(LOB_PARANOID)
endif()

add_executable(lob_demo src/main.cpp)

include(CTest)
if(BUILD_TESTING)
    add_executable(price_level_tests tests/priceLevelTests.cpp)
    add_test(NAME price_level_tests COMMAND price_level_tests)
endif()
