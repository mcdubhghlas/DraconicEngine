cmake_minimum_required(VERSION 4.0)

project(DraconicEngine LANGUAGES C CXX)

find_package(Git QUIET)
find_package(Threads REQUIRED)

if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    message(STATUS "Updating submodules...")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GIT_SUBMOD_RESULT
    )
    
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
        message(FATAL_ERROR "Failed to update submodules. Please run manually.")
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

option(BUILD_BENCHMARKS "Build micro-benchmarks (nanobench)" OFF)

include(CTest)
include(Compiler)
include(Modules)
include(Shaders)

# Engine runtime (C++)
add_subdirectory(Engine/cpp)

# Samples (C++)
add_subdirectory(Samples/cpp)

# Aggregate benchmark targets: `benchmarks` builds them all, `run-benchmarks`
# builds and runs each in turn.
if(BUILD_BENCHMARKS)
    get_property(DRACO_BENCH_TARGETS GLOBAL PROPERTY DRACO_BENCH_TARGETS)
    if(DRACO_BENCH_TARGETS)
        add_custom_target(benchmarks)
        add_dependencies(benchmarks ${DRACO_BENCH_TARGETS})

        set(BENCH_RUN_CMDS "")
        foreach(BENCH_TARGET IN LISTS DRACO_BENCH_TARGETS)
            list(APPEND BENCH_RUN_CMDS COMMAND $<TARGET_FILE:${BENCH_TARGET}>)
        endforeach()
        add_custom_target(run-benchmarks
            ${BENCH_RUN_CMDS}
            DEPENDS ${DRACO_BENCH_TARGETS}
            USES_TERMINAL
            VERBATIM
        )
    endif()
endif()
