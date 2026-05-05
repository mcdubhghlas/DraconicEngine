module;

#include <bgfx/bgfx.h>

module rendering.rendergraph;

import rendering.rhi;

namespace draco::rendering::rendergraph {

    void RenderGraph::reset() {
        m_passes.clear();
    }

    Pass& RenderGraph::add_pass() {
        m_passes.emplace_back();
        return m_passes.back();
    }

    void RenderGraph::execute() {
        for (auto& pass : m_passes) {

            std::println("[RenderGraph] Executing pass: {}", pass.name); // Debug

            draco::rendering::rhi::apply_view(pass.view, {pass.framebuffer, 0, 0, pass.width, pass.height, pass.clear_flags, pass.clear_color});

            draco::rendering::rhi::set_view_projection(pass.view, pass.view_mtx, pass.proj_mtx);

            if (pass.clear_flags != 0) {
                bgfx::setViewClear(pass.view, pass.clear_flags, pass.clear_color);
            }

            for (auto& pkt : pass.packets) {
                draco::rendering::rhi::submit(pkt, pass.view);
            }
        }
    }
}