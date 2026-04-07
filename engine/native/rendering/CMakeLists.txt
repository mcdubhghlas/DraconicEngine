add_modules_library(rhi SHARED)
target_sources(rhi PRIVATE rhi/rhi_bgfx.cpp rhi/vertex.cppm)
target_link_libraries(rhi PUBLIC core bgfx bx)
