module;

#include <cstdint>

export module rendering.rendergraph;

import rendering.rhi;
import std;

export namespace draco::rendering::rendergraph {

    struct Pass {
        std::string name;

        draco::rendering::rhi::ViewID view;
        draco::rendering::rhi::FramebufferHandle framebuffer;

        std::vector<draco::rendering::rhi::RenderPacket> packets;

        float view_mtx[16];
        float proj_mtx[16];

        uint16_t width;
        uint16_t height;

        uint32_t clear_flags;
        uint32_t clear_color;
    };

    class RenderGraph {
    public:
        void reset();

        Pass& add_pass();

        void execute();
    private:
        std::vector<Pass> m_passes;
    };
}
