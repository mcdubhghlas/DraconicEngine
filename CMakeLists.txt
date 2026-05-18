cmake_minimum_required(VERSION 4.0)

find_package(Git QUIET)

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

project(DraconicEngine LANGUAGES C CXX)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    add_compile_options(-mavx2 -mfma)
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Subdirectories for engine libs
add_subdirectory(engine/native)

# Main Executable
add_executable(draconic engine/native/main/main.cpp)

enable_modules(draconic)

set_target_properties(draconic PROPERTIES CXX_SCAN_FOR_MODULES ON)

target_link_libraries(draconic
    PRIVATE
    core
    io
    memory
    image_loader
    platform
    input
    rhi
    renderer
    rendergraph
    mesh
    material
    quad_renderer
    scene
    camera # Camera controller
    transform
    renderable
    bgfx
    bx
    bimg
)

find_package(Threads REQUIRED)

if(UNIX AND NOT APPLE)
    target_link_libraries(draconic PRIVATE SDL3::SDL3-static Threads::Threads dl)
    target_link_options(draconic PRIVATE -Wl,--whole-archive $<TARGET_FILE:SDL3::SDL3-static> -Wl,--no-whole-archive)
elseif(APPLE)
    target_link_libraries(draconic PRIVATE SDL3::SDL3-static)
    target_link_options(draconic PRIVATE -Wl,-force_load,$<TARGET_FILE:SDL3::SDL3-static>)
else()
    target_link_libraries(draconic PRIVATE SDL3::SDL3-static)
endif()

set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/engine/native/rendering/shaders")
set(SHADER_BIN_DIR "${CMAKE_BINARY_DIR}")

file(MAKE_DIRECTORY ${SHADER_BIN_DIR})

if(APPLE)
    set(VERTEX_PROFILE "metal")
    set(FRAGMENT_PROFILE "metal")
    set(SHADER_PLATFORM "osx")
elseif(WIN32)
    set(VERTEX_PROFILE "spirv")
    set(FRAGMENT_PROFILE "spirv")
    set(SHADER_PLATFORM "windows")
else()
    set(VERTEX_PROFILE "spirv")
    set(FRAGMENT_PROFILE "spirv")
    set(SHADER_PLATFORM "linux")
endif()

set(BGFX_INCLUDE "${CMAKE_SOURCE_DIR}/engine/native/thirdparty/bgfx/src")

set(VERTEX_INPUT "${SHADER_SRC_DIR}/vs.sc")
set(VERTEX_OUTPUT "${SHADER_BIN_DIR}/vs.bin")

set(FRAGMENT_INPUT "${SHADER_SRC_DIR}/fs.sc")
set(FRAGMENT_OUTPUT "${SHADER_BIN_DIR}/fs.bin")

set(QUAD_VERTEX_INPUT "${SHADER_SRC_DIR}/vs_quad.sc")
set(QUAD_VERTEX_OUTPUT "${SHADER_BIN_DIR}/vs_quad.bin")

set(QUAD_FRAGMENT_INPUT "${SHADER_SRC_DIR}/fs_quad.sc")
set(QUAD_FRAGMENT_OUTPUT "${SHADER_BIN_DIR}/fs_quad.bin")

set(VARYING_DEF "${SHADER_SRC_DIR}/varying.def.sc")
set(QUAD_VARYING_DEF "${SHADER_SRC_DIR}/varying_quad.def.sc")

add_custom_command(
    TARGET draconic POST_BUILD
    COMMENT "Compiling asset pipelines and core engine shaders via native tools..."

    COMMAND $<TARGET_FILE:shaderc>
        -f ${VERTEX_INPUT}
        -o ${VERTEX_OUTPUT}
        --type vertex
        --platform ${SHADER_PLATFORM}
        -p ${VERTEX_PROFILE}
        --varyingdef ${VARYING_DEF}
        -i ${BGFX_INCLUDE}

    COMMAND $<TARGET_FILE:shaderc>
        -f ${FRAGMENT_INPUT}
        -o ${FRAGMENT_OUTPUT}
        --type fragment
        --platform ${SHADER_PLATFORM}
        -p ${FRAGMENT_PROFILE}
        --varyingdef ${VARYING_DEF}
        -i ${BGFX_INCLUDE}

    COMMAND $<TARGET_FILE:shaderc>
        -f ${QUAD_VERTEX_INPUT}
        -o ${QUAD_VERTEX_OUTPUT}
        --type vertex
        --platform ${SHADER_PLATFORM}
        -p ${VERTEX_PROFILE}
        --varyingdef ${QUAD_VARYING_DEF}
        -i ${BGFX_INCLUDE}

    COMMAND $<TARGET_FILE:shaderc>
        -f ${QUAD_FRAGMENT_INPUT}
        -o ${QUAD_FRAGMENT_OUTPUT}
        --type fragment
        --platform ${SHADER_PLATFORM}
        -p ${VERTEX_PROFILE}
        --varyingdef ${QUAD_VARYING_DEF}
        -i ${BGFX_INCLUDE}

    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VERTEX_OUTPUT} $<TARGET_FILE_DIR:draconic>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FRAGMENT_OUTPUT} $<TARGET_FILE_DIR:draconic>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QUAD_VERTEX_OUTPUT} $<TARGET_FILE_DIR:draconic>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QUAD_FRAGMENT_OUTPUT} $<TARGET_FILE_DIR:draconic>
)

set(TEST_IMAGE
    "${CMAKE_SOURCE_DIR}/assets/draconic_logo_no_text.png"
)

add_custom_command(
    TARGET draconic POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${TEST_IMAGE}
        $<TARGET_FILE_DIR:draconic>/test.png

    COMMENT "Copying test texture..."
)
