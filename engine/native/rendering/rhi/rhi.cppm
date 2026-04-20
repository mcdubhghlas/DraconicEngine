module;

#include <cstdint>

export module rendering.rhi;

import rendering.rhi.vertex; // For the vertex layout

export namespace draco::rhi
{
    using BufferHandle   = uint16_t;
    using PipelineHandle = uint16_t;
    using ShaderHandle   = uint16_t;
    using UniformHandle  = uint16_t;
    using TextureHandle  = uint16_t;
    using FramebufferHandle = uint16_t;
    using ViewID         = uint16_t;

    inline constexpr BufferHandle InvalidBuffer = 0xFFFF;
    inline constexpr PipelineHandle InvalidPipeline = 0xFFFF;
    inline constexpr ShaderHandle InvalidShader = 0xFFFF;
    inline constexpr UniformHandle InvalidUniform = 0xFFFF;
    inline constexpr TextureHandle InvalidTexture = 0xFFFF;
    inline constexpr FramebufferHandle InvalidFramebuffer = 0xFFFF;
    inline constexpr ViewID InvalidView = 0xFFFF;

    enum class PipelineState : uint64_t {
        Default = 0,
        WriteRGB = 1 << 0,
        WriteAlpha = 1 << 1,
        MSAA = 1 << 2,
        PrimitiveTriStrip = 1 << 3,
    };

    enum class UniformType
    {
        Sampler, // For textures
        Vec4, // For colors or simple data
        Mat3, // For normal matrices
        Mat4, // For transform matrices
    };

    enum class TextureFormat {
        RGBA8,
        D16,
        D24,
        D32
    };

    struct TransientBuffer {
        const void* data;
        uint32_t size;
        uint16_t stride;
    };

    struct RenderPacket
    {
        uint64_t sort_key;

        BufferHandle vertex_buffer = InvalidBuffer;
        BufferHandle index_buffer = InvalidBuffer;
        PipelineHandle pipeline = InvalidPipeline;
        UniformHandle uniform_handle = InvalidUniform;
        TextureHandle texture_handle = InvalidTexture;
        
        const void* uniform_data = nullptr;

        uint8_t texture_unit = 0; // Which slot the texture goes into

        float model[16];
        uint32_t draw_tags;
    };

    struct PipelineDesc
    {
        ShaderHandle vs;
        ShaderHandle fs;
        PipelineState state;
    };

    bool init(void* display_type, void* window_handle, uint16_t width, uint16_t height);
    void shutdown();

    void resize(uint16_t width, uint16_t height);

    // Create the shader from the given data & size
    // Note: The data should be in the format expected by the underlying graphics API
    ShaderHandle create_shader(const void* data, uint32_t size);

    // Create a pipeline from the give description
    PipelineHandle create_pipeline(const PipelineDesc&);

    // Create a vertex or index buffer from the given data & size
    BufferHandle create_vertex_buffer(const void* data, uint32_t size);
    BufferHandle create_index_buffer(const void* data, uint32_t size);

    // Create a uniform with the given name, type & count
    UniformHandle create_uniform(const char* name, UniformType type, uint16_t num = 1);
    void destroy_uniform(UniformHandle handle);

    // Set the value of a uniform with the given handle, value & count
    void set_uniform(UniformHandle handle, const void* value, uint16_t num = 1);

    // Create a texture from the given data, width, height & flags
    TextureHandle create_texture(const void* data, uint16_t width, uint16_t height, uint32_t flags = 0);
    void destroy_texture(TextureHandle handle);

    FramebufferHandle create_framebuffer(uint16_t width, uint16_t height, TextureFormat format);
    void destroy_framebuffer(FramebufferHandle handle);
    TextureHandle get_framebuffer_texture(FramebufferHandle handle);

    // Dynamic Buffer functions (For data that changes often)
    BufferHandle create_dynamic_vertex_buffer(uint32_t size, uint16_t stride);
    void update_dynamic_vertex_buffer(BufferHandle handle, uint32_t start_vertex, const void* data, uint32_t size);

    // Set the render target for a specific view
    void set_view_framebuffer(ViewID view, FramebufferHandle handle);
    
    // Helper function to set a 4x4 matrix to the identity matrix
    void identity_matrix(float* _mtx);

    // Submit a render packet to the given view for rendering
    void submit(const RenderPacket&, ViewID);

    // Begin and end the rendering of a frame
    void begin_frame();
    void end_frame();

    constexpr PipelineState operator|(PipelineState a, PipelineState b) {
        return static_cast<PipelineState>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
    }

    constexpr PipelineState operator&(PipelineState a, PipelineState b) {
        return static_cast<PipelineState>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
    }
}