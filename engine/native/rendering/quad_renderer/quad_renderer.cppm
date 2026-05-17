module;

#include <vector>
#include <cstdint>
#include <cmath>

export module rendering.quad_renderer;

import rendering.rhi;
import rendering.rhi.vertex;
import rendering.rendergraph;

export namespace draco::rendering::quad_renderer {

    struct BatchKey {
        draco::rendering::rhi::TextureHandle texture = draco::rendering::rhi::InvalidTexture;

        draco::rendering::rhi::PipelineHandle pipeline = draco::rendering::rhi::InvalidPipeline;

        draco::rendering::rhi::SamplerHandle sampler = draco::rendering::rhi::InvalidSampler;

        bool operator==(const BatchKey&) const = default;
    };

    struct QuadCommand {
        draco::rendering::rhi::TextureHandle texture = draco::rendering::rhi::InvalidTexture;

        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        float width  = 1.0f;
        float height = 1.0f;

        float rotation = 0.0f;

        uint32_t color = 0xffffffff;
    };

    struct OrthoCamera {
        float view[16];
        float proj[16];

        float x = 0.0f;
        float y = 0.0f;
        float zoom = 1.0f;
    };

    class QuadRenderer {
    public:
        static constexpr uint32_t MaxQuads    = 10000;
        static constexpr uint32_t MaxVertices = MaxQuads * 4;
        static constexpr uint32_t MaxIndices  = MaxQuads * 6;

        void init(draco::rendering::rhi::PipelineHandle pipeline);

        void begin();

        void submit(const QuadCommand& cmd);

        void flush_to_pass(draco::rendering::rendergraph::Pass& pass);

        void shutdown();

        static void build_ortho(OrthoCamera& cam, float width, float height);

    private:
        void push_quad(const QuadCommand& cmd);

    private:
        BatchKey m_batch_key{};

        std::vector<draco::rendering::rhi::TexturedVertex> m_vertices;

        std::vector<uint16_t> m_indices;

        draco::rendering::rhi::BufferHandle m_vb = draco::rendering::rhi::InvalidBuffer;

        draco::rendering::rhi::BufferHandle m_ib = draco::rendering::rhi::InvalidBuffer;

        draco::rendering::rhi::LayoutHandle m_layout = draco::rendering::rhi::InvalidLayout;

        draco::rendering::rhi::PipelineHandle m_pipeline = draco::rendering::rhi::InvalidPipeline;

        draco::rendering::rhi::UniformHandle m_sampler = draco::rendering::rhi::InvalidUniform;

        uint32_t m_quad_count = 0;
    };
}
