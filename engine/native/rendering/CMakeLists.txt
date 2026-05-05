add_modules_library(rhi)

target_sources(rhi
    PUBLIC
        FILE_SET CXX_MODULES FILES
            rhi/vertex.cppm
            rhi/rhi.cppm
    PRIVATE
        rhi/rhi_bgfx.cpp
)

target_link_libraries(rhi
    PUBLIC
        core
        memory
        bgfx
        bx
)

add_modules_library(rendergraph)

target_sources(rendergraph
    PUBLIC
        FILE_SET CXX_MODULES FILES
            rendergraph/rendergraph.cppm
    PRIVATE
        rendergraph/rendergraph.cpp
)

target_link_libraries(rendergraph
    PUBLIC
        rhi
        bx
)

add_modules_library(renderer)

target_sources(renderer
    PUBLIC
        FILE_SET CXX_MODULES FILES
            renderer/renderer.cppm
    PRIVATE
        renderer/renderer.cpp
)

target_link_libraries(renderer
    PUBLIC
        core
        rhi
        rendergraph
        bgfx
        bx
)