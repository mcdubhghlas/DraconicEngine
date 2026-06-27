include_guard(GLOBAL)

set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/Engine/cpp/runtime/Rendering/Shaders")
set(SHADER_BIN_DIR "${CMAKE_BINARY_DIR}")
set(BGFX_INCLUDE "${CMAKE_SOURCE_DIR}/Engine/cpp/ThirdParty/bgfx/src")

function(compile_shaders TARGET_NAME)
    file(MAKE_DIRECTORY ${SHADER_BIN_DIR})

    if(APPLE)
        set(VERTEX_PROFILE "metal")
        set(FRAGMENT_PROFILE "metal")
        set(SHADER_PLATFORM "osx")
    elseif(WIN32)
        set(VERTEX_PROFILE "s_5_0")
        set(FRAGMENT_PROFILE "s_5_0")
        set(SHADER_PLATFORM "windows")
    else()
        set(VERTEX_PROFILE "spirv")
        set(FRAGMENT_PROFILE "spirv")
        set(SHADER_PLATFORM "linux")
    endif()

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
        TARGET ${TARGET_NAME} POST_BUILD
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
            -p ${FRAGMENT_PROFILE}
            --varyingdef ${QUAD_VARYING_DEF}
            -i ${BGFX_INCLUDE}

        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VERTEX_OUTPUT} $<TARGET_FILE_DIR:${TARGET_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FRAGMENT_OUTPUT} $<TARGET_FILE_DIR:${TARGET_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QUAD_VERTEX_OUTPUT} $<TARGET_FILE_DIR:${TARGET_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QUAD_FRAGMENT_OUTPUT} $<TARGET_FILE_DIR:${TARGET_NAME}>
    )
endfunction()
