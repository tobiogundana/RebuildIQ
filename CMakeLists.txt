
cmake_minimum_required(VERSION 3.20)

project(RebuildIQ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(recovery src/recovery.cpp)

target_include_directories(recovery PUBLIC include)

add_executable(rebuildiq src/main.cpp)

target_link_libraries(rebuildiq PRIVATE recovery)

enable_testing()

add_executable(recovery_tests tests/test_recovery.cpp)

target_link_libraries(recovery_tests PRIVATE recovery)

if(MSVC)
    target_compile_options(recovery_tests PRIVATE /UNDEBUG)
else()
    target_compile_options(recovery_tests PRIVATE -UNDEBUG)
endif()

add_test(NAME RecoveryTests COMMAND recovery_tests)
