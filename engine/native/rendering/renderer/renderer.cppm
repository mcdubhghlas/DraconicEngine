module;

#include <bx/math.h>

export module rendering.renderer;

import std;
import rendering.rhi;
import rendering.rendergraph;

export namespace draco::rendering::renderer {

    struct Camera {
        std::array<float, 3> position;
        std::array<float, 3> target;
        std::array<float, 3> up;
        float fov;
        float near_plane;
        float far_plane;
    };

    struct SceneContext {
        uint16_t screen_width;
        uint16_t screen_height;
        Camera main_camera;

        draco::rendering::rendergraph::RenderGraph graph;
    };

    SceneContext g_ctx;

    void init(uint16_t width, uint16_t height);
    void resize(uint16_t width, uint16_t height);
    
    void begin_frame(const Camera& cam);

    void submit_entity(draco::rendering::rhi::RenderPacket& packet, uint16_t view);

    void end_frame();
}