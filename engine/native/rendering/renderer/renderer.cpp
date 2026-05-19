module;

#include <vector>
#include <cstdint>
#include <cstring>
#include <print>
#include <algorithm>

#include <bx/math.h>
#include <bgfx/bgfx.h>

module rendering.renderer;

import rendering.rhi;
import rendering.rhi.uniform_registry;
import rendering.rendergraph;
import rendering.mesh;
import rendering.material;
import rendering.quad_renderer;

import scene;
import scene.transform;

namespace draco::rendering::renderer
{
    static constexpr const char* MAIN_PASS = "MainPass";

    void init(uint16_t width, uint16_t height)
    {
        g_ctx.screen_width = width;
        g_ctx.screen_height = height;
    }

    void resize(uint16_t width, uint16_t height)
    {
        g_ctx.screen_width = width;
        g_ctx.screen_height = height;
    }

    void begin_frame(const Camera& cam)
    {
        rhi::begin_frame();

        g_ctx.main_camera = cam;
        g_ctx.graph.reset();

        // Create main pass once per frame
        auto& pass = g_ctx.graph.add_pass(MAIN_PASS);

        pass.view = 0;
        pass.framebuffer = rhi::InvalidFramebuffer;

        pass.width  = g_ctx.screen_width;
        pass.height = g_ctx.screen_height;

        pass.clear_flags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH;
        pass.clear_color = 0x303030ff;

        float view_mtx[16];
        float proj_mtx[16];

        rhi::look_at(view_mtx, cam.position.data(), cam.target.data(), cam.up.data());

        float aspect = float(g_ctx.screen_width) / float(std::max<uint16_t>(g_ctx.screen_height, 1));

        rhi::perspective(proj_mtx, cam.fov, aspect, cam.near_plane, cam.far_plane);

        std::memcpy(pass.view_mtx, view_mtx, sizeof(view_mtx));
        std::memcpy(pass.proj_mtx, proj_mtx, sizeof(proj_mtx));
    }

    static void build_uniforms(const material::Material& mat, std::vector<rhi::UniformBind>& out)
    {
        out.clear();
        out.reserve(mat.uniforms.size());

        for (const auto& u : mat.uniforms)
        {
            rhi::UniformBind bind{};

            bind.handle = rhi::get_uniform(u.name_hash);

            bind.data = u.data;
            bind.num  = u.count;

            if (bind.handle == rhi::InvalidUniform)
            {
                std::println("[Renderer] Missing uniform hash: {}", u.name_hash);
                continue;
            }

            out.push_back(bind);
        }
    }

    void submit_entity(const rhi::RenderPacket& packet)
    {
        auto* pass = g_ctx.graph.get_pass(MAIN_PASS);
        if (!pass) return;

        pass->packets.push_back(packet);
    }

    void render_scene(const draco::scene::Scene& scene)
    {
        for (auto& t : scene.renderables)
        {
            const auto* mesh = mesh::get(t.mesh);
            if (!mesh) continue;

            rhi::RenderPacket p{};

            p.vertex_buffer = mesh->vbh;
            p.index_buffer  = mesh->ibh;

            p.pipeline        = t.material.pipeline;
            p.texture_handle  = t.material.texture;
            p.texture_unit    = t.material.texture_unit;
            p.sampler_uniform = t.material.sampler;

            build_uniforms(t.material, p.uniforms);

            float model[16];
            draco::scene::transform::compute_matrix(t.transform, model);

            std::memcpy(p.model, model, sizeof(model));

            submit_entity(p);
        }
    }

    void submit_ui(draco::rendering::quad_renderer::QuadRenderer& quad_renderer)
    {
        auto& ui_pass = g_ctx.graph.add_pass("UIPass");

        ui_pass.view = 1;
        ui_pass.sort_mode = rendergraph::SortMode::None;

        ui_pass.framebuffer = rhi::InvalidFramebuffer;

        ui_pass.width  = g_ctx.screen_width;
        ui_pass.height = g_ctx.screen_height;

        ui_pass.clear_flags = 0;

        draco::rendering::quad_renderer::OrthoCamera ortho;

        draco::rendering::quad_renderer::QuadRenderer::build_ortho(ortho, (float)g_ctx.screen_width, (float)g_ctx.screen_height);

        std::memcpy(ui_pass.view_mtx, ortho.view, sizeof(float) * 16);
        std::memcpy(ui_pass.proj_mtx, ortho.proj, sizeof(float) * 16);

        quad_renderer.flush_to_pass(ui_pass);
    }

    void end_frame()
    {
        g_ctx.graph.execute();
        rhi::end_frame();
    }

    rendergraph::RenderGraph& get_graph()
    {
        return draco::rendering::renderer::g_ctx.graph;
    }
}
