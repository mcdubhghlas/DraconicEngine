module;

#include <vector>
#include <array>
#include <string>
#include <cstdint>
#include <functional>
#include <cstring>
#include <algorithm>

#include <bx/math.h>

export module rendering.renderer;

import rendering.rhi;
import rendering.rendergraph;
import rendering.quad_renderer;

import scene;

export namespace draco::rendering::renderer {

    struct Camera {
        std::array<float, 3> position {0.0f, 0.0f, 0.0f};
        std::array<float, 3> target {0.0f, 0.0f, 0.0f};
        std::array<float, 3> up {0.0f, 1.0f, 0.0f};
        float fov = 60.0f;
        float near_plane = 0.1f;
        float far_plane = 1000.0f;
    };

    struct SceneContext {
        uint16_t screen_width = 0;
        uint16_t screen_height = 0;
        Camera main_camera;

        draco::rendering::rendergraph::RenderGraph graph;
    };

    inline SceneContext g_ctx;

    void init(uint16_t width, uint16_t height);
    void resize(uint16_t width, uint16_t height);

    void begin_frame(const Camera& cam);

    void submit_entity(draco::rendering::rhi::RenderPacket& packet, uint16_t view);

    void end_frame();

    void render_scene(const draco::scene::Scene& scene);

    void submit_ui(draco::rendering::quad_renderer::QuadRenderer& quad_renderer);

    rendergraph::RenderGraph& get_graph();
}
