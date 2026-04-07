export module rendering.rhi;

#include <cstdint>

export namespace draco::rhi
{
    using BufferHandle   = uint16_t;
    using PipelineHandle = uint16_t;
    using ShaderHandle   = uint16_t;
    using ViewID         = uint16_t;

    struct RenderPacket
    {
        uint64_t sort_key;

        BufferHandle vertex_buffer;
        BufferHandle index_buffer;
        PipelineHandle pipeline;

        float model[16];
        uint32_t draw_tags;
    };

    struct PipelineDesc
    {
        ShaderHandle vs;
        ShaderHandle fs;
        uint64_t state;
    };

    bool init(void* display_type, void* window_handle, uint16_t width, uint16_t height);
    void shutdown();

    void resize(uint16_t width, uint16_t height);

    ShaderHandle create_shader(const void* data, uint32_t size);
    PipelineHandle create_pipeline(const PipelineDesc&);

    BufferHandle create_vertex_buffer(const void* data, uint32_t size);
    BufferHandle create_index_buffer(const void* data, uint32_t size);

    void submit(const RenderPacket&, ViewID);

    void begin_frame();
    void end_frame();
}