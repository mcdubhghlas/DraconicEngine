module;

#include <bx/math.h>
#include <bgfx/bgfx.h>

import std;
import rendering.rhi;
import rendering.rendergraph;

module rendering.renderer;

namespace draco::rendering::renderer
{
    void init(uint16_t width, uint16_t height) {
        g_ctx.screen_width = width;
        g_ctx.screen_height = height;
    }

    void resize(uint16_t width, uint16_t height) {
        g_ctx.screen_width = width;
        g_ctx.screen_height = height;
    }

    void begin_frame(const Camera& cam) {
        draco::rendering::rhi::begin_frame();

        g_ctx.main_camera = cam;

        g_ctx.graph.reset();
    }
    
    void submit_entity(draco::rendering::rhi::RenderPacket& packet, uint16_t view)
    {
        auto& main_pass = g_ctx.graph.add_pass();

        main_pass.name = "MainPass";
        main_pass.view = view;
        main_pass.framebuffer = draco::rendering::rhi::InvalidFramebuffer;

        main_pass.width  = g_ctx.screen_width;
        main_pass.height = g_ctx.screen_height;

        main_pass.clear_flags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH;
        main_pass.clear_color = 0x303030ff;

        // Build camera every frame
        float view_mtx[16];
        float proj_mtx[16];

        draco::rendering::rhi::look_at(view_mtx, g_ctx.main_camera.position.data(), g_ctx.main_camera.target.data(), g_ctx.main_camera.up.data());

        float aspect = float(g_ctx.screen_width) / float(g_ctx.screen_height);

        draco::rendering::rhi::perspective(proj_mtx, g_ctx.main_camera.fov, aspect, g_ctx.main_camera.near_plane, g_ctx.main_camera.far_plane);

        std::memcpy(main_pass.view_mtx, view_mtx, sizeof(view_mtx));
        std::memcpy(main_pass.proj_mtx, proj_mtx, sizeof(proj_mtx));

        main_pass.packets.push_back(packet);
    }

    void end_frame() {
        g_ctx.graph.execute();

        draco::rendering::rhi::end_frame();
    }
}