module;

#include <vector>
#include <cstdint>

export module rendering.material;

import rendering.rhi;

export namespace draco::rendering::material
{
    struct Uniform
    {
        uint32_t name_hash = 0;
        const void* data = nullptr;
        uint16_t count = 1;
    };

    struct Material
    {
        uint32_t shader_id = 0;

        rhi::PipelineHandle pipeline = rhi::InvalidPipeline;

        rhi::TextureHandle texture = rhi::InvalidTexture;
        rhi::UniformHandle sampler = rhi::InvalidUniform;

        uint8_t texture_unit = 0;

        std::vector<Uniform> uniforms;
    };
}
