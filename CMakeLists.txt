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

set(SHADER_DIR "${CMAKE_SOURCE_DIR}/engine/native/rendering/shaders")
set(BGFX_INCLUDE "${CMAKE_SOURCE_DIR}/engine/native/thirdparty/bgfx/src")

if(WIN32)

    find_program(SHADERC NAMES shadercRelease.exe shaderc PATHS "${CMAKE_SOURCE_DIR}/engine/native/thirdparty/bgfx/.build"
        PATH_SUFFIXES
            win64_vs2022/bin
            win64_vs2019/bin
            win64_mingw-gcc/bin
        NO_DEFAULT_PATH
    )

    set(SHADER_PLATFORM windows)
    set(SHADER_PROFILE spirv)

elseif(APPLE)

    find_program(SHADERC NAMES shadercRelease shaderc PATHS "${CMAKE_SOURCE_DIR}/engine/native/thirdparty/bgfx/.build"
        PATH_SUFFIXES
            osx-arm64_clang/bin
            osx-x64_clang/bin
        NO_DEFAULT_PATH
    )

    set(SHADER_PLATFORM osx)
    set(SHADER_PROFILE metal)

elseif(UNIX)

    find_program(SHADERC NAMES shadercRelease shaderc PATHS "${CMAKE_SOURCE_DIR}/engine/native/thirdparty/bgfx/.build"
        PATH_SUFFIXES
            linux64_gcc/bin
            linux64_clang/bin
        NO_DEFAULT_PATH
    )

    set(SHADER_PLATFORM linux)
    set(SHADER_PROFILE spirv)

endif()

if(SHADERC)

    message(STATUS "shaderc found: ${SHADERC}")

    set(VERTEX_SHADER        "${SHADER_DIR}/vs.sc")
    set(FRAGMENT_SHADER      "${SHADER_DIR}/fs.sc")
    set(VARYING_DEF          "${SHADER_DIR}/varying.def.sc")

    set(QUAD_VERTEX_SHADER   "${SHADER_DIR}/vs_quad.sc")
    set(QUAD_FRAGMENT_SHADER "${SHADER_DIR}/fs_quad.sc")
    set(QUAD_VARYING_DEF     "${SHADER_DIR}/varying_quad.def.sc")

    set(VERTEX_OUTPUT        "${CMAKE_BINARY_DIR}/vs.bin")
    set(FRAGMENT_OUTPUT      "${CMAKE_BINARY_DIR}/fs.bin")

    set(QUAD_VERTEX_OUTPUT   "${CMAKE_BINARY_DIR}/vs_quad.bin")
    set(QUAD_FRAGMENT_OUTPUT "${CMAKE_BINARY_DIR}/fs_quad.bin")

    add_custom_command(
        OUTPUT ${VERTEX_OUTPUT}

        COMMAND ${SHADERC}
            -f ${VERTEX_SHADER}
            -o ${VERTEX_OUTPUT}
            --type vertex
            --platform ${SHADER_PLATFORM}
            -p ${SHADER_PROFILE}
            --varyingdef ${VARYING_DEF}
            -i ${BGFX_INCLUDE}

        DEPENDS
            ${VERTEX_SHADER}
            ${VARYING_DEF}

        COMMENT "Compiling vertex shader..."
    )

    add_custom_command(
        OUTPUT ${FRAGMENT_OUTPUT}

        COMMAND ${SHADERC}
            -f ${FRAGMENT_SHADER}
            -o ${FRAGMENT_OUTPUT}
            --type fragment
            --platform ${SHADER_PLATFORM}
            -p ${SHADER_PROFILE}
            --varyingdef ${VARYING_DEF}
            -i ${BGFX_INCLUDE}

        DEPENDS
            ${FRAGMENT_SHADER}
            ${VARYING_DEF}

        COMMENT "Compiling fragment shader..."
    )

    add_custom_command(
        OUTPUT ${QUAD_VERTEX_OUTPUT}

        COMMAND ${SHADERC}
            -f ${QUAD_VERTEX_SHADER}
            -o ${QUAD_VERTEX_OUTPUT}
            --type vertex
            --platform ${SHADER_PLATFORM}
            -p ${SHADER_PROFILE}
            --varyingdef ${QUAD_VARYING_DEF}
            -i ${BGFX_INCLUDE}

        DEPENDS
            ${QUAD_VERTEX_SHADER}
            ${QUAD_VARYING_DEF}

        COMMENT "Compiling quad vertex shader..."
    )

    add_custom_command(
        OUTPUT ${QUAD_FRAGMENT_OUTPUT}

        COMMAND ${SHADERC}
            -f ${QUAD_FRAGMENT_SHADER}
            -o ${QUAD_FRAGMENT_OUTPUT}
            --type fragment
            --platform ${SHADER_PLATFORM}
            -p ${SHADER_PROFILE}
            --varyingdef ${QUAD_VARYING_DEF}
            -i ${BGFX_INCLUDE}

        DEPENDS
            ${QUAD_FRAGMENT_SHADER}
            ${QUAD_VARYING_DEF}

        COMMENT "Compiling quad fragment shader..."
    )

    add_custom_target(shaders ALL
        DEPENDS
            ${VERTEX_OUTPUT}
            ${FRAGMENT_OUTPUT}
            ${QUAD_VERTEX_OUTPUT}
            ${QUAD_FRAGMENT_OUTPUT}
    )

    add_dependencies(draconic shaders)

    add_custom_command(
        TARGET draconic POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${VERTEX_OUTPUT}
            $<TARGET_FILE_DIR:draconic>

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${FRAGMENT_OUTPUT}
            $<TARGET_FILE_DIR:draconic>

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${QUAD_VERTEX_OUTPUT}
            $<TARGET_FILE_DIR:draconic>

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${QUAD_FRAGMENT_OUTPUT}
            $<TARGET_FILE_DIR:draconic>

        COMMENT "Copying compiled shaders..."
    )

else()

    message(WARNING
        "shaderc not found. "
        "Shaders will not be compiled automatically."
    )

endif()

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
