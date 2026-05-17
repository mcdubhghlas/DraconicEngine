module;

#include <vector>
#include <array>
#include <string>
#include <cstdint>
#include <functional>
#include <cstring>
#include <algorithm>
#include <print>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>

module rendering.rhi;

namespace draco::rendering::rhi
{
    #ifndef DRACO_RHI_VALIDATION
    #define DRACO_RHI_VALIDATION 1
    #endif

    #if DRACO_RHI_VALIDATION
        #define RHI_ASSERT(cond, msg, ...) \
            do { \
                if (!(cond)) { \
                    std::println("[RHI ERROR] " msg, ##__VA_ARGS__); \
                    std::abort(); \
                } \
            } while(0)

        #define RHI_WARN(cond, msg, ...) \
            do { \
                if (!(cond)) { \
                    std::println("[RHI WARNING] " msg, ##__VA_ARGS__); \
                } \
            } while(0)
    #else
        #define RHI_ASSERT(cond, msg, ...) do { (void)(cond); } while(0)
        #define RHI_WARN(cond, msg, ...)   do { (void)(cond); } while(0)
    #endif

    using namespace draco::core::memory;

    static HandleRegistry<Buffer, BufferTag> g_buffers;
    static HandleRegistry<Pipeline, PipelineTag> g_pipelines;
    static HandleRegistry<bgfx::UniformHandle, UniformTag> g_uniforms;
    static HandleRegistry<bgfx::TextureHandle, TextureTag> g_textures;
    static HandleRegistry<FramebufferResource, FramebufferTag> g_framebuffers;
    static HandleRegistry<bgfx::ShaderHandle, ShaderTag> g_shaders;
    static HandleRegistry<VertexLayoutResource, LayoutTag> g_layouts;

    // Deferred destruction queue (GPU-safe deletion)
    static std::vector<DeletionReq> g_deletion_queue;

    static uint16_t g_width = 0;
    static uint16_t g_height = 0;

    // Ensures a handle is valid before use
    // TODO: Replace with something better
    template<typename Registry, typename HandleT>
    auto* get_checked(Registry& reg, HandleT h, const char* name)
    {
        if (!reg.valid(h))
        {
            RHI_WARN(false, "{} handle invalid or stale!", name);
            return (decltype(reg.get(h)))nullptr;
        }

        return reg.get(h);
    }

    // GPU-safe destruction (delayed by 2 frames)
    // This is the industry standard 
    template<typename T>
    void destroy_later(T handle)
    {
        if (!bgfx::isValid(handle))
            return;

        g_deletion_queue.push_back({
            bgfx::getStats()->gpuFrameNum,
            [handle]() { bgfx::destroy(handle); }
        });
    }

    void process_deletions()
    {
        uint64_t frame = bgfx::getStats()->gpuFrameNum;

        std::erase_if(g_deletion_queue, [frame](const auto& d)
        {
            if (frame >= d.frame + 2)
            {
                d.cleanup();
                return true;
            }
            return false;
        });
    }

    bool init(void* display_type, void* window_handle, draco::platform::NativeWindowType window_type, uint16_t width, uint16_t height)
    {
        g_width = width;
        g_height = height;

        bgfx::Init init{};
        init.type = bgfx::RendererType::Count;

        init.platformData.ndt = display_type;
        init.platformData.nwh = window_handle;

        // Map our internal window type to bgfx's native window handle type
        if (window_type == draco::platform::NativeWindowType::Wayland)
        {
            init.platformData.type = bgfx::NativeWindowHandleType::Wayland;
        }
        else
        {
            // Others can work fine with the default type
            init.platformData.type = bgfx::NativeWindowHandleType::Default;
        }

        init.resolution.width  = width;
        init.resolution.height = height;
        init.resolution.reset  = BGFX_RESET_VSYNC;

        if (!bgfx::init(init))
        {
            RHI_WARN(false, "bgfx initialization failed");
            return false;
        }

        bgfx::setDebug(BGFX_DEBUG_TEXT);
        return true;
    }

    void resize(uint16_t width, uint16_t height)
    {
        if(width == 0 || height == 0)
            return; // Minimized window safety

        if(width == g_width && height == g_height)
            return; // No need to resize
        
        g_width = width;
        g_height = height;

        bgfx::reset(width, height, BGFX_RESET_VSYNC);
    }

    void shutdown()
    {
        // Walk all registries and destroy live GPU objects
        for (auto& slot : g_buffers.internal().raw())
        {
            if (!slot.alive) continue;

            if (bgfx::isValid(slot.value.vbh))
                bgfx::destroy(slot.value.vbh);

            if (bgfx::isValid(slot.value.ibh))
                bgfx::destroy(slot.value.ibh);
            
            if (bgfx::isValid(slot.value.dvbh))
                bgfx::destroy(slot.value.dvbh);
        }

        for (auto& slot : g_pipelines.internal().raw())
        {
            if (!slot.alive) continue;

            if (bgfx::isValid(slot.value.program))
                bgfx::destroy(slot.value.program);
        }

        for (auto& slot : g_uniforms.internal().raw())
        {
            if (!slot.alive) continue;

            if (bgfx::isValid(slot.value))
                bgfx::destroy(slot.value);
        }

        for (auto& slot : g_textures.internal().raw())
        {
            if (!slot.alive) continue;

            if (bgfx::isValid(slot.value))
                bgfx::destroy(slot.value);
        }

        bgfx::shutdown();
    }

    PipelineHandle create_pipeline(const PipelineDesc& desc)
    {
        RHI_ASSERT(desc.vs != InvalidShader, "Pipeline missing vertex shader");
        RHI_ASSERT(desc.fs != InvalidShader, "Pipeline missing fragment shader");

        bgfx::ProgramHandle prog = bgfx::createProgram(resolve(desc.vs), resolve(desc.fs), true);

        uint64_t state = map_state(desc.state, desc.blend, desc.depth, desc.cull, desc.depth_write);

        return g_pipelines.create({ prog, state });
    }

    BufferHandle create_vertex_buffer(const void* data, uint32_t size, LayoutHandle layout_h)
    {
        RHI_ASSERT(data != nullptr, "Vertex buffer data is null");
        RHI_ASSERT(size > 0, "Vertex buffer size is zero");

        auto* layout = get_checked(g_layouts, layout_h, "Layout");

        RHI_ASSERT(layout, "Invalid vertex layout");

        auto vbh = bgfx::createVertexBuffer(bgfx::copy(data, size), layout->layout);

        Buffer buf;
        buf.vbh = vbh;

        return g_buffers.create(buf);
    }

    BufferHandle create_index_buffer(const void* data, uint32_t size)
    {
        RHI_ASSERT(data != nullptr, "Index buffer data is null");
        RHI_ASSERT(size > 0, "Index buffer size is zero");

        bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(bgfx::copy(data, size), BGFX_BUFFER_INDEX32);

        Buffer buf; // Idk why I named it this, it just sounds funny ;)
        buf.ibh = ibh;
        buf.is_index = true;

        return g_buffers.create(buf);
    }

    BufferHandle create_dynamic_vertex_buffer(uint32_t size, LayoutHandle layout_h)
    {
        auto* layout = get_checked(g_layouts, layout_h, "Layout");
        RHI_ASSERT(layout, "Invalid layout");

        bgfx::DynamicVertexBufferHandle dvbh = bgfx::createDynamicVertexBuffer(size, layout->layout);

        RHI_ASSERT(bgfx::isValid(dvbh), "Failed to create dynamic vertex buffer");

        Buffer buf;
        buf.dvbh = dvbh;
        buf.is_dynamic = true;

        return g_buffers.create(buf);
    }

    void update_dynamic_vertex_buffer(BufferHandle handle, uint32_t start_vertex, const void* data, uint32_t size)
    {
        auto* buf = get_checked(g_buffers, handle, "Buffer");

        if (!buf)
            return;

        RHI_ASSERT(buf->is_dynamic && !buf->is_index, "Not a dynamic vertex buffer");
        RHI_ASSERT(bgfx::isValid(buf->dvbh), "Invalid dynamic vertex buffer handle");

        const bgfx::Memory* mem = bgfx::copy(data, size);

        bgfx::update(buf->dvbh, start_vertex, mem);
    }

    BufferHandle create_dynamic_index_buffer(uint32_t size, uint16_t flags)
    {
        bgfx::DynamicIndexBufferHandle ibh = bgfx::createDynamicIndexBuffer(size, flags);

        RHI_ASSERT(bgfx::isValid(ibh), "Invalid dynamic index buffer handle");

        Buffer buf{};
        buf.is_dynamic = true;
        buf.is_index = true;
        buf.dibh = ibh;

        return g_buffers.create(buf);
    }

    void update_dynamic_index_buffer(BufferHandle handle, uint32_t start_index, const void* data, uint32_t size)
    {
        auto* buf = get_checked(g_buffers, handle, "DynamicIndexBuffer");

        if (!buf)
            return;

        RHI_ASSERT(buf->is_dynamic && buf->is_index, "Not a dynamic index buffer");

        const bgfx::Memory* mem = bgfx::copy(data, size);

        bgfx::update(buf->dibh, start_index, mem);
    }

    void destroy_buffer(BufferHandle h)
    {
        auto* buf = get_checked(g_buffers, h, "Buffer");

        if (!buf)
            return;

        if (bgfx::isValid(buf->vbh))
            destroy_later(buf->vbh);

        if (bgfx::isValid(buf->ibh))
            destroy_later(buf->ibh);

        if (bgfx::isValid(buf->dvbh))
            destroy_later(buf->dvbh);
        
        if (bgfx::isValid(buf->dibh))
            destroy_later(buf->dibh);

        g_buffers.destroy(h);
    }

    LayoutHandle create_vertex_layout(const VertexLayoutDesc& desc)
    {
        bgfx::VertexLayout layout;
        layout.begin();

        for (const auto& e : desc.elements)
        {
            layout.add(map_attrib(e.attrib), e.count, map_attrib_type(e.type), e.normalized);
        }

        layout.end();

        return g_layouts.create({ layout });
    }

    UniformHandle create_uniform(const char* name, UniformType type, uint16_t num)
    {
        RHI_ASSERT(name != nullptr, "Uniform name is null");

        auto u = bgfx::createUniform(name, map_uniform_type(type), num);
        return g_uniforms.create(u);
    }

    void set_uniform(UniformHandle h, const void* data, uint16_t num)
    {
        auto* u = get_checked(g_uniforms, h, "Uniform");
        if (!u) return;

        RHI_ASSERT(data != nullptr, "Uniform data is null");

        bgfx::setUniform(*u, data, num);
    }

    void destroy_uniform(UniformHandle h)
    {
        auto* u = get_checked(g_uniforms, h, "Uniform");
        if (!u) return;

        destroy_later(*u);
        g_uniforms.destroy(h);
    }

    TextureHandle create_texture(const void* data, uint32_t w, uint32_t h, uint32_t flags)
    {
        RHI_ASSERT(data != nullptr, "Texture data is null");
        RHI_ASSERT(w > 0 && h > 0, "Invalid texture dimensions");

        auto tex = bgfx::createTexture2D(
            w, h, false, 1,
            bgfx::TextureFormat::RGBA8,
            flags == 0 ? (BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP) : flags,
            bgfx::copy(data, w * h * 4)
        );

        return g_textures.create(tex);
    }

    ShaderHandle create_shader(const void* data, uint32_t size)
    {
        RHI_ASSERT(data && size > 0, "Invalid shader data");

        bgfx::ShaderHandle sh = bgfx::createShader(bgfx::copy(data, size));

        return g_shaders.create(sh);
    }

    bgfx::ShaderHandle resolve(ShaderHandle h)
    {
        auto* sh = g_shaders.get(h);
        return sh ? *sh : bgfx::ShaderHandle{ bgfx::kInvalidHandle };
    }

    // For debugging/tooling
    bgfx::ShaderHandle* get_shader_native(ShaderHandle h)
    {
        return get_checked(g_shaders, h, "Shader");
    }

    void destroy_shader(ShaderHandle h)
    {
        if (auto* sh = get_checked(g_shaders, h, "Shader"))
        {
            destroy_later(*sh);
            g_shaders.destroy(h);
        }
    }

    void perspective(float* out, float fov, float aspect, float nearp, float farp)
    {
        bx::mtxProj(out, fov, aspect, nearp, farp, bgfx::getCaps()->homogeneousDepth);
    }

    void look_at(float* out, const float* eye, const float* at, const float* up)
    {
        bx::Vec3 eye_v { eye[0], eye[1], eye[2] };
        bx::Vec3 at_v  { at[0],  at[1],  at[2]  };
        bx::Vec3 up_v  { up[0],  up[1],  up[2]  };

        bx::mtxLookAt(out, eye_v, at_v, up_v);
    }

     // Note: Internal use only, use apply_view() instead
    void set_view_framebuffer(ViewID view, FramebufferHandle h)
    {
        auto* fb = get_checked(g_framebuffers, h, "Framebuffer");

        if (!fb)
            return;

        bgfx::setViewFrameBuffer(view, fb->fbh);
    }

     // Note: Internal use only, use apply_view() instead
    void set_view_rect(ViewID view, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
    {
       bgfx::setViewRect(view, x, y, w, h);
    }

    void set_view_projection(ViewID view, const float* view_mtx, const float* proj_mtx)
    {
        bgfx::setViewTransform(view, view_mtx, proj_mtx);
    }

    void set_scissor(const ScissorRect& r)
    {
        if (!r.enabled)
            bgfx::setScissor(UINT16_MAX);
        else
            bgfx::setScissor(r.x, r.y, r.w, r.h);
    }

    void set_stencil(uint32_t fstencil, uint32_t bstencil)
    {
        bgfx::setStencil(fstencil, bstencil);
    }

    uint64_t map_state(PipelineState s, BlendMode blend, DepthTest depth, CullMode cull, bool depth_write)
    {
        uint64_t state = 0;

        if ((s & PipelineState::WriteRGB) != PipelineState::Default)
            state |= BGFX_STATE_WRITE_RGB;

        if ((s & PipelineState::WriteAlpha) != PipelineState::Default)
            state |= BGFX_STATE_WRITE_A;

        if (depth_write)
            state |= BGFX_STATE_WRITE_Z;

        switch (depth)
        {
            case DepthTest::Less:   state |= BGFX_STATE_DEPTH_TEST_LESS; break;
            case DepthTest::Equal:  state |= BGFX_STATE_DEPTH_TEST_EQUAL; break;
            case DepthTest::Always: state |= BGFX_STATE_DEPTH_TEST_ALWAYS; break;
            case DepthTest::None:   break;
        }

    
        switch (cull)
        {
            case CullMode::CW:  state |= BGFX_STATE_CULL_CW; break;
            case CullMode::CCW: state |= BGFX_STATE_CULL_CCW; break;
            case CullMode::None: break;
        }

        switch (blend)
        {
            case BlendMode::Alpha:
                state |= BGFX_STATE_BLEND_ALPHA;
                break;

            case BlendMode::Additive:
                state |= BGFX_STATE_BLEND_ADD;
                break;

            case BlendMode::Multiply:
                state |= BGFX_STATE_BLEND_MULTIPLY;
                break;

            case BlendMode::None:
                break;
        }

        if ((s & PipelineState::MSAA) != PipelineState::Default)
            state |= BGFX_STATE_MSAA;

        if ((s & PipelineState::PrimitiveTriStrip) != PipelineState::Default)
            state |= BGFX_STATE_PT_TRISTRIP;

        return state;
    }

    bgfx::UniformType::Enum map_uniform_type(UniformType t)
    {
        switch (t)
        {
            case UniformType::Sampler: return bgfx::UniformType::Sampler;
            case UniformType::Vec4:    return bgfx::UniformType::Vec4;
            case UniformType::Mat3:    return bgfx::UniformType::Mat3;
            case UniformType::Mat4:    return bgfx::UniformType::Mat4;
        }
        return bgfx::UniformType::Vec4;
    }

    bgfx::Attrib::Enum map_attrib(Attrib a)
    {
        switch (a)
        {
            case Attrib::Position:  return bgfx::Attrib::Position;
            case Attrib::Color0:    return bgfx::Attrib::Color0;
            case Attrib::TexCoord0: return bgfx::Attrib::TexCoord0;
            case Attrib::Normal:    return bgfx::Attrib::Normal;
            case Attrib::Tangent:   return bgfx::Attrib::Tangent;
        }

        return bgfx::Attrib::Position;
    }

    bgfx::AttribType::Enum map_attrib_type(AttribType t)
    {
        switch (t)
        {
            case AttribType::Float:
                return bgfx::AttribType::Float;

            case AttribType::Uint8:
                return bgfx::AttribType::Uint8;
        }

        return bgfx::AttribType::Float;
    }

    void apply_view(ViewID view, const ViewDesc& desc)
    {
        if (desc.fb != InvalidFramebuffer)
        {
            auto* fb = get_checked(g_framebuffers, desc.fb, "Framebuffer");

            if (fb && bgfx::isValid(fb->fbh))
            {
                bgfx::setViewFrameBuffer(view, fb->fbh);
            }
            else
            {
                RHI_WARN(false, "Framebuffer invalid at apply_view");
            }
        }

        bgfx::setViewRect(view, desc.x, desc.y, desc.w, desc.h);

        if (desc.clear_flags != 0)
        {
            bgfx::setViewClear(view, desc.clear_flags, desc.clear_color);
        }
    }

    void identity_matrix(float* mtx)
    {
        bx::mtxIdentity(mtx);
    }

    void submit(const RenderPacket& p, ViewID view)
    {
        auto* pipeline = get_checked(g_pipelines, p.pipeline, "Pipeline");
        auto* vb = get_checked(g_buffers, p.vertex_buffer, "VertexBuffer");
        Buffer* ib = nullptr;

        if (!pipeline || !vb)
            return;
        
        if (p.index_buffer != InvalidBuffer)
            ib = get_checked(g_buffers, p.index_buffer, "IndexBuffer");

        // Transform matrix (model)
        bgfx::setTransform(p.model);

        // Vertex buffer binding with explicit range control
        if (vb->is_dynamic)
        {
            // If count is UINT32_MAX, bgfx will fallback to drawing the full buffer automatically
            bgfx::setVertexBuffer(0, vb->dvbh, 0, p.vertex_count);
        } else {
            bgfx::setVertexBuffer(0, vb->vbh, 0, p.vertex_count);
        }

        // Index buffer binding with explicit range control
        if (ib && ib->is_index)
        {
            if (ib->is_dynamic)
            {
                bgfx::setIndexBuffer(ib->dibh, 0, p.index_count);
            } else {
                bgfx::setIndexBuffer(ib->ibh, 0, p.index_count);
            }
        }

        // Uniforms
        for (const auto& u : p.uniforms)
        {
            if (auto* handle = get_checked(g_uniforms, u.handle, "UniformBind"))
            {
                bgfx::setUniform(*handle, u.data, u.num);
            }
        }

        // Texture binding
        if (auto* tex = get_checked(g_textures, p.texture_handle, "Texture"))
        {
            if (auto* sampler = get_checked(g_uniforms, p.sampler_uniform, "Sampler"))
            {
                bgfx::setTexture(p.texture_unit, *sampler, *tex, p.sampler_flags);
            }
        }

        // Apply pipeline state & submit draw call
        bgfx::setState(pipeline->state);
        bgfx::submit(view, pipeline->program);
    }

    void begin_frame()
    {
        // Clean up GPU resources safely
        process_deletions();
    }

    void end_frame()
    {
        // Submit frame to GPU
        bgfx::frame();
    }
}
