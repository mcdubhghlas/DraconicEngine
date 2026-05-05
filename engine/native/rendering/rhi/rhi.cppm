module;

#include <cstdint>
#include <bgfx/bgfx.h>

export module rendering.rhi;

import std;
import core.memory;
import rendering.rhi.vertex;

export namespace draco::rendering::rhi
{
    struct BufferTag {};
    struct PipelineTag {};
    struct ShaderTag {};
    struct UniformTag {};
    struct TextureTag {};
    struct FramebufferTag {};
    struct LayoutTag {};

    using BufferHandle      = core::memory::Handle<BufferTag>;
    using PipelineHandle    = core::memory::Handle<PipelineTag>;
    using ShaderHandle      = core::memory::Handle<ShaderTag>;
    using UniformHandle     = core::memory::Handle<UniformTag>;
    using TextureHandle     = core::memory::Handle<TextureTag>;
    using FramebufferHandle = core::memory::Handle<FramebufferTag>;
    using LayoutHandle      = core::memory::Handle<LayoutTag>;

    using ViewID        = uint16_t;   // bgfx native
    using SamplerHandle = uint64_t;   // bgfx sampler flags

    inline constexpr BufferHandle      InvalidBuffer{};
    inline constexpr PipelineHandle    InvalidPipeline{};
    inline constexpr ShaderHandle      InvalidShader{};
    inline constexpr UniformHandle     InvalidUniform{};
    inline constexpr TextureHandle     InvalidTexture{};
    inline constexpr FramebufferHandle InvalidFramebuffer{};
    inline constexpr LayoutHandle      InvalidLayout{};
    inline constexpr SamplerHandle     InvalidSampler = 0;
    inline constexpr ViewID            InvalidView = UINT16_MAX;

    enum class PipelineState : uint64_t {
        Default             = 0,
        WriteRGB            = 1ULL << 0,
        WriteAlpha          = 1ULL << 1,
        MSAA                = 1ULL << 2,
        PrimitiveTriStrip   = 1ULL << 3,
    };

    enum class ClearFlags : uint32_t {
        Color   = BGFX_CLEAR_COLOR,
        Depth   = BGFX_CLEAR_DEPTH,
        Stencil = BGFX_CLEAR_STENCIL
        };

    struct ViewDesc {
        FramebufferHandle fb = InvalidFramebuffer;
        uint16_t x, y, w, h;
        uint32_t clear_flags;
        uint32_t clear_color;
    };

    enum class UniformType
    {
        Sampler,
        Vec4,
        Mat3,
        Mat4,
    };

    struct UniformBind {
        UniformHandle handle;
        const void* data;
        uint16_t num;
    };

    enum class TextureFormat {
        RGBA8,
        BGRA8,
        D16,
        D24,
        D24S8,
        D32
    };

    enum class BlendMode {
        None,
        Alpha,
        Additive,
        Multiply
    };

    enum class DepthTest {
        None,
        Less,
        Equal,
        Always
    };

    enum class CullMode {
        None,
        CW,
        CCW
    };

    struct Buffer
    {
        bgfx::VertexBufferHandle vbh = BGFX_INVALID_HANDLE;
        bgfx::DynamicVertexBufferHandle dvbh = BGFX_INVALID_HANDLE;
        bgfx::IndexBufferHandle  ibh = BGFX_INVALID_HANDLE;
        bool is_dynamic = false;
        bool is_index = false;
    };

    struct FramebufferResource {
        bgfx::FrameBufferHandle fbh;
        TextureHandle texture;
    };

    struct VertexLayoutResource
    {
        bgfx::VertexLayout layout;
    };

    struct ScissorRect {
        uint16_t x, y, w, h;
        bool enabled = true;
    };

    struct DeletionReq {
        uint64_t frame;
        std::function<void()> cleanup;
    };

    struct PipelineDesc
    {
        ShaderHandle vs;
        ShaderHandle fs;
        PipelineState state;

        BlendMode blend = BlendMode::None;
        DepthTest depth = DepthTest::Less;
        CullMode  cull  = CullMode::CCW;

        bool depth_write = true;
    };

    struct RenderPacket
    {
        uint64_t sort_key = 0;

        BufferHandle vertex_buffer = InvalidBuffer;
        BufferHandle index_buffer  = InvalidBuffer;
        PipelineHandle pipeline    = InvalidPipeline;

        UniformHandle sampler_uniform = InvalidUniform;
        SamplerHandle sampler_flags   = InvalidSampler;
        TextureHandle texture_handle  = InvalidTexture;

        float color[4] = {1,1,1,1};

        std::vector<UniformBind> uniforms;
        uint8_t texture_unit = 0;

        float model[16] = {
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1
        };

        uint32_t draw_tags = 0;
    };

    struct Pipeline {
        bgfx::ProgramHandle program;
        uint64_t state;
    };

    bool init(void* display_type, void* window_handle, uint16_t width, uint16_t height);
    void resize(uint16_t width, uint16_t height);
    void shutdown();

    PipelineHandle create_pipeline(const PipelineDesc&);

    BufferHandle create_vertex_buffer(const void* data, uint32_t size);
    BufferHandle create_index_buffer(const void* data, uint32_t size);

    UniformHandle create_uniform(const char* name, UniformType type, uint16_t num = 1);
    void destroy_uniform(UniformHandle handle);
    void set_uniform(UniformHandle handle, const void* value, uint16_t num = 1);

    TextureHandle create_texture(const void* data, uint16_t width, uint16_t height, uint32_t flags = 0);
    void destroy_texture(TextureHandle handle);

    FramebufferHandle create_framebuffer(uint16_t width, uint16_t height, TextureFormat format);
    void destroy_framebuffer(FramebufferHandle handle);
    TextureHandle get_framebuffer_texture(FramebufferHandle handle);

    BufferHandle create_dynamic_vertex_buffer(uint32_t size, LayoutHandle layout);
    void update_dynamic_vertex_buffer(BufferHandle handle, uint32_t start_vertex, const void* data, uint32_t size);

    LayoutHandle create_vertex_layout(const VertexLayoutDesc& desc);

    SamplerHandle create_sampler(bool linear, bool clamp);

    // Expects bgfx compiled shader binary (shaderc output)
    ShaderHandle create_shader(const void* data, uint32_t size);
    bgfx::ShaderHandle resolve(ShaderHandle h);
    // For debugging/tooling
    bgfx::ShaderHandle* get_shader_native(ShaderHandle h);
    void destroy_shader(ShaderHandle h);

    void perspective(float* out, float fov, float aspect, float nearp, float farp);
    void look_at(float* out, const float* eye, const float* at, const float* up);

    // Note: Internal use only, use apply_view() instead
    void set_view_rect(ViewID view, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void set_view_framebuffer(ViewID view, FramebufferHandle handle);

    void set_view_projection(ViewID view, const float* view_mtx, const float* proj_mtx);
    void set_scissor(const ScissorRect& r);
    void set_stencil(uint32_t f_stencil, uint32_t b_stencil);

    void apply_view(ViewID view, const ViewDesc& desc);

    void identity_matrix(float* _mtx);

    uint64_t map_state(PipelineState s, BlendMode, DepthTest, CullMode, bool depth_write);
    bgfx::UniformType::Enum map_uniform_type(UniformType t);

    void submit(const RenderPacket&, ViewID);

    void begin_frame();
    void end_frame();

    template<typename T>
    void destroy_resource(T handle);

    void process_deletions();

    constexpr PipelineState operator|(PipelineState a, PipelineState b) {
        return static_cast<PipelineState>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
    }

    constexpr PipelineState operator&(PipelineState a, PipelineState b) {
        return static_cast<PipelineState>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
    }
}
