#
# Rendering Sample
#
add_executable(RenderSample RenderSample.cpp)

# Everything the sample needs comes through Runtime (which links the single Shell
# library, whose SDL3 dependency propagates for the createShell() backend).
target_link_libraries(RenderSample
        PRIVATE
        Runtime
        bgfx
        bx
        bimg
)

# SDL is pulled in (statically) through the Shell library; force-load the whole archive
# at the executable so SDL's constructor-registered video/input drivers are not stripped
# by the linker. This stays on the final binary rather than on Shell, so it does not also
# force a full SDL copy into the intermediate Runtime shared library.
if(UNIX AND NOT APPLE)
    target_link_libraries(RenderSample PRIVATE Threads::Threads dl)
    target_link_options(RenderSample PRIVATE -Wl,--whole-archive $<TARGET_FILE:SDL3::SDL3-static> -Wl,--no-whole-archive)
elseif(APPLE)
    target_link_options(RenderSample PRIVATE -Wl,-force_load,$<TARGET_FILE:SDL3::SDL3-static>)
elseif(MSVC)
    target_link_options(RenderSample PRIVATE /WHOLEARCHIVE:$<TARGET_FILE:SDL3::SDL3-static>)
endif()

compile_shaders(RenderSample)

set(TEST_IMAGE
        "${CMAKE_SOURCE_DIR}/Docs/assets/draconic_logo_no_text.png"
)

add_custom_command(
        TARGET RenderSample POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${TEST_IMAGE}
        $<TARGET_FILE_DIR:RenderSample>/test.png

        COMMENT "Copying test texture..."
)
