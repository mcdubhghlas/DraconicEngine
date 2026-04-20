module;

import std;

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>

module rendering.rhi;

namespace draco::rhi
{
    struct Buffer
    {
        bgfx::VertexBufferHandle vbh = BGFX_INVALID_HANDLE; // Stores vertex data, but it can be null if this buffer is an index buffer
        bgfx::IndexBufferHandle  ibh = BGFX_INVALID_HANDLE; // Stores index data, but it can be null if this buffer is a vertex buffer
        bool is_index = false; // Allows us to know which type of buffer this is without needing to check the handles
    };

    struct Pipeline
    {
        bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
        uint64_t state = 0; // Stored as raw bgfx bitmask
    };

    // Static globals
    // Note: We need to replace it with a more robust resource management system later, but this'll work for now
    static std::vector<Buffer>   g_buffers;
    static std::vector<Pipeline> g_pipelines;
    static std::vector<bgfx::UniformHandle> g_uniforms;
    static std::vector<bgfx::TextureHandle> g_textures;
    static std::vector<bgfx::FrameBufferHandle> g_framebuffers;
    static std::vector<bgfx::DynamicVertexBufferHandle> g_dynamic_vbs;
    static std::map<FramebufferHandle, TextureHandle> g_fb_to_tex;

    static uint16_t g_width = 0;
    static uint16_t g_height = 0;

    bool init(void* display_type, void* window_handle, uint16_t width, uint16_t height)
    {
        g_width = width;
        g_height = height;

        bgfx::Init init{};

        // TODO: Replace this
        init.type = bgfx::RendererType::Count; // Auto-selects the renderer
        
        // Pass the handles directly into the init struct
        // So, ndt is the native display type & nwh is the native window handle
        init.platformData.ndt = display_type;
        init.platformData.nwh = window_handle;
        
        init.resolution.width  = width;
        init.resolution.height = height;
        init.resolution.reset  = BGFX_RESET_VSYNC;

        if (!bgfx::init(init)) {
            std::println("bgfx failed to init!");
            return false;
        }

        bgfx::setDebug(BGFX_DEBUG_TEXT);
        bgfx::setViewClear(0,
            BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
            0x303030ff, 1.0f, 0);
            
        return true;
    }

    void shutdown()
    {
        for (auto& b : g_buffers)
        {
            // Destroy the buffers
            if (bgfx::isValid(b.vbh)) bgfx::destroy(b.vbh);
            if (bgfx::isValid(b.ibh)) bgfx::destroy(b.ibh);
        }

        for (auto& p : g_pipelines)
        {
            // Destroy the pipeline
            if (bgfx::isValid(p.program)) bgfx::destroy(p.program);
        }

        for (auto& u : g_uniforms)
        {
            // Destroy the uniforms
            if (bgfx::isValid(u)) bgfx::destroy(u);
        }

        for (auto& t : g_textures)
        {
            // Destroy the textures
            if (bgfx::isValid(t)) bgfx::destroy(t);
        }

        // Clear everything so we don't have dangling handles
        g_buffers.clear();
        g_pipelines.clear();
        g_uniforms.clear();
        g_textures.clear();

        // Have bgfx destroy the context and everything it holds internally
        bgfx::shutdown();
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

    uint64_t map_state(PipelineState state)
    {
        uint64_t bgfx_state = BGFX_STATE_NONE;
        // Cast to uint64_t so the 'if' can treat it as a boolean check
        // Otherwise, the bitwise check would fail since PipelineState is an enum class and doesn't implicitly convert to uint64_t
        if (static_cast<uint64_t>(state & PipelineState::WriteRGB)) bgfx_state |= BGFX_STATE_WRITE_RGB;
        if (static_cast<uint64_t>(state & PipelineState::WriteAlpha)) bgfx_state |= BGFX_STATE_WRITE_A;
        if (static_cast<uint64_t>(state & PipelineState::MSAA)) bgfx_state |= BGFX_STATE_MSAA;
        if (static_cast<uint64_t>(state & PipelineState::PrimitiveTriStrip)) bgfx_state |= BGFX_STATE_PT_TRISTRIP;
        return bgfx_state;
    }

    ShaderHandle create_shader(const void* data, uint32_t size)
    {
        // Check the input data before trying to create the shader
        // If it's invalid, print an error & return an invalid handle except for crashing
        if(!data || size == 0)
        {
            std::println("Error: Invalid shader data or size");
            return InvalidShader;
        }

        const bgfx::Memory* mem = bgfx::copy(data, size);
        return bgfx::createShader(mem).idx;
    }

    PipelineHandle create_pipeline(const PipelineDesc& desc)
    {
        // Check the input data before trying to create the pipeline
        // If it's invalid, print an error & return an invalid handle except for crashing
        if(!desc.vs || !desc.fs)
        {
            std::println("Error: Invalid shader handles in PipelineDesc");
            return InvalidPipeline;
        }


        bgfx::ShaderHandle vs{ desc.vs };
        bgfx::ShaderHandle fs{ desc.fs };

        bgfx::ProgramHandle prog = bgfx::createProgram(vs, fs, true);

        g_pipelines.push_back({ prog, map_state(desc.state) });
        return (PipelineHandle)(g_pipelines.size() - 1);
    }

    BufferHandle create_vertex_buffer(const void* data, uint32_t size)
    {
        // Check the input data before trying to create the buffer
        // If it's invalid, print an error & return an invalid handle except for crashing
        if(!data || size == 0)
        {
            std::println("Error: Invalid vertex buffer data or size");
            return InvalidBuffer;
        }

        bgfx::VertexLayout layout;
        layout.begin()
            .add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true) // normalized
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

        bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
            bgfx::copy(data, size),
            layout
        );

        g_buffers.push_back({ vbh, BGFX_INVALID_HANDLE, false });
        return (BufferHandle)(g_buffers.size() - 1);
    }

    BufferHandle create_index_buffer(const void* data, uint32_t size)
    {
        // Check the input data before trying to create the buffer
        // If it's invalid, print an error & return an invalid handle except for crashing
        if(!data || size == 0)
        {
            std::println("Error: Invalid index buffer data or size");
            return InvalidBuffer;
        }

        bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(
            bgfx::copy(data, size)
        );

        g_buffers.push_back({ BGFX_INVALID_HANDLE, ibh, true });
        return (BufferHandle)(g_buffers.size() - 1);
    }

    static bgfx::UniformType::Enum map_uniform_type(UniformType type) {
        switch (type) {
            case UniformType::Sampler: return bgfx::UniformType::Sampler;
            case UniformType::Vec4:    return bgfx::UniformType::Vec4;
            case UniformType::Mat3:    return bgfx::UniformType::Mat3;
            case UniformType::Mat4:    return bgfx::UniformType::Mat4;
        }
        return bgfx::UniformType::Count;
    }

    UniformHandle create_uniform(const char* name, UniformType type, uint16_t num) {
        bgfx::UniformHandle handle = bgfx::createUniform(name, map_uniform_type(type), num);
        g_uniforms.push_back(handle);
        return static_cast<UniformHandle>(g_uniforms.size() - 1);
    }

    void set_uniform(UniformHandle handle, const void* value, uint16_t num) {
        // Check for null/invalid handles
        if (handle == InvalidUniform) return; 

        // Check for out of bound handles
        if (handle >= g_uniforms.size()) {
            std::println("Error: Uniform handle out of bounds!");
            return;
        }

        bgfx::setUniform(g_uniforms[handle], value, num);
    }

    void destroy_uniform(UniformHandle handle) {
        if (handle < g_uniforms.size() && bgfx::isValid(g_uniforms[handle])) {
            bgfx::destroy(g_uniforms[handle]);
            // We don't remove from vector to keep indices stable, 
            // just invalidate the handle
            g_uniforms[handle] = BGFX_INVALID_HANDLE;
        }
    }

    bgfx::TextureFormat::Enum map_format(TextureFormat format) {
        switch(format) {
            case TextureFormat::RGBA8: return bgfx::TextureFormat::RGBA8;
            case TextureFormat::D24:   return bgfx::TextureFormat::D24;
            default:                   return bgfx::TextureFormat::RGBA8;
        }
    }

    TextureHandle create_texture(const void* data, uint16_t width, uint16_t height, uint32_t flags)
    {
        if (!data || width == 0 || height == 0) {
            std::println("Error: Invalid texture data or dimensions");
            return InvalidTexture;
        }

        // Default flags are linear filtering & clamp to edge
        uint64_t bgfx_flags = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT;
        
        const bgfx::Memory* mem = bgfx::copy(data, width * height * 4); // Assuming RGBA8, but this should be calculated based on the actual format
        bgfx::TextureHandle handle = bgfx::createTexture2D(
            width, height, false, 1, 
            bgfx::TextureFormat::RGBA8, 
            bgfx_flags, mem
        );

        g_textures.push_back(handle);
        return static_cast<TextureHandle>(g_textures.size() - 1);
    }

    FramebufferHandle create_framebuffer(uint16_t width, uint16_t height, TextureFormat format) {
        // Create the texture that will be the color attachment
        // We use BGFX_TEXTURE_RT to tell bgfx this is a Render Target
        bgfx::TextureHandle th = bgfx::createTexture2D(
            width, height, false, 1, bgfx::TextureFormat::RGBA8, 
            BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP
        );

        // Create the framebuffer using that texture
        bgfx::FrameBufferHandle fbh = bgfx::createFrameBuffer(1, &th, true);
        draco::rhi::g_framebuffers.push_back(fbh);
    
        draco::rhi::FramebufferHandle handle = static_cast<draco::rhi::FramebufferHandle>(draco::rhi::g_framebuffers.size() - 1);
    
        // Register the texture
        draco::rhi::g_textures.push_back(th);
        draco::rhi::TextureHandle tex_handle = static_cast<draco::rhi::TextureHandle>(draco::rhi::g_textures.size() - 1);
    
        draco::rhi::g_fb_to_tex[handle] = tex_handle;
    
        return handle;
    }

    TextureHandle get_framebuffer_texture(FramebufferHandle handle) {
        if (draco::rhi::g_fb_to_tex.contains(handle)) {
            return draco::rhi::g_fb_to_tex[handle];
        }
        return draco::rhi::InvalidTexture;
    }

    void set_view_framebuffer(ViewID view, FramebufferHandle handle) {
        if (handle == draco::rhi::InvalidFramebuffer) {
            bgfx::setViewFrameBuffer(view, { bgfx::kInvalidHandle });
        } else {
            bgfx::setViewFrameBuffer(view, draco::rhi::g_framebuffers[handle]);
        }
    }
    
    BufferHandle create_dynamic_vertex_buffer(uint32_t size) {
        bgfx::VertexLayout layout;
        layout.begin()
            .add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

        bgfx::DynamicVertexBufferHandle dvb = bgfx::createDynamicVertexBuffer(size / 24, layout);
        // For now, let's just push it to a new specialized vector
        g_dynamic_vbs.push_back(dvb);
        return static_cast<BufferHandle>(g_dynamic_vbs.size() - 1);
    }

    void submit_transient(const RenderPacket& p, ViewID view, const void* data, uint32_t size) {
        bgfx::TransientVertexBuffer tvb;
        bgfx::VertexLayout layout;
        layout.begin()
            .add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

        // Allocate from the transient pool for this frame
        bgfx::allocTransientVertexBuffer(&tvb, size / 24, layout);
    
        if (tvb.data != nullptr) { // Check if the pointer is valid
            std::memcpy(tvb.data, data, size);
        }
        bgfx::setVertexBuffer(0, &tvb);
        
        // Apply uniforms, state, etc.
        bgfx::setState(g_pipelines[p.pipeline].state);
        bgfx::submit(view, g_pipelines[p.pipeline].program);
    }

    void identity_matrix(float* _mtx)
    {
        bx::mtxIdentity(_mtx);
    }

    void submit(const draco::rhi::RenderPacket& p, ViewID view)
    {
        // Check for null/invalid handles
        if (p.pipeline == draco::rhi::InvalidPipeline || p.vertex_buffer == draco::rhi::InvalidBuffer) {
            std::println("Error: Attempted to submit RenderPacket with unitialized handles.");
            return;
        }

        // Check for out of bounds handles
        if (p.pipeline >= draco::rhi::g_pipelines.size() || p.vertex_buffer >= draco::rhi::g_buffers.size()) {
            std::println("Error: Handle out of bounds! The resource may have been destroyed already or it was never created. Pipeline Handle: {}, Vertex Buffer Handle: {}", p.pipeline, p.vertex_buffer);
            return;
        }

        draco::rhi::Pipeline& pipeline = draco::rhi::g_pipelines[p.pipeline];
        draco::rhi::Buffer& vb = draco::rhi::g_buffers[p.vertex_buffer];

        bgfx::setTransform(p.model);
        bgfx::setVertexBuffer(0, draco::rhi::g_buffers[p.vertex_buffer].vbh);

        if (p.index_buffer != draco::rhi::InvalidBuffer)
        {
            if (p.index_buffer >= draco::rhi::g_buffers.size()) {
                std::println("Error: Invalid index buffer handle in RenderPacket");
                return;
            }
            draco::rhi::Buffer& ib = draco::rhi::g_buffers[p.index_buffer];
            if (ib.is_index)
                bgfx::setIndexBuffer(ib.ibh);
        }

        if (p.uniform_handle != draco::rhi::InvalidUniform && p.uniform_data != nullptr)
        {
            bgfx::setUniform(draco::rhi::g_uniforms[p.uniform_handle], p.uniform_data, 1);
        }

        // Bind Texture if valid
        if (p.texture_handle != draco::rhi::InvalidTexture && p.texture_handle < draco::rhi::g_textures.size())
        {
            // TODO: Use a dedicated sampler handle later 
            // For now, we'll assume the uniform_handle passed is the sampler.
            if (p.uniform_handle != draco::rhi::InvalidUniform) {
                bgfx::setTexture(p.texture_unit, draco::rhi::g_uniforms[p.uniform_handle], draco::rhi::g_textures[p.texture_handle]);
            }
        }

        bgfx::setState(pipeline.state);
        bgfx::submit(view, pipeline.program);
    }

    void begin_frame()
    {
        // Initialize View 0 (Offscreen)
        bgfx::setViewRect(0, 0, 0, g_width, g_height);
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

        // Initialize View 1 (Backbuffer/Screen)
        bgfx::setViewRect(1, 0, 0, g_width, g_height);
        bgfx::setViewClear(1, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);

        // Set transforms for both views
        float view[16];
        bx::mtxIdentity(view);
        float proj[16];
        bx::mtxOrtho(proj, -1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 100.0f, 0.0f, bgfx::getCaps()->homogeneousDepth);

        bgfx::setViewTransform(0, view, proj);
        bgfx::setViewTransform(1, view, proj);

        // Touch both views so they definitely render
        bgfx::touch(0);
        bgfx::touch(1);
    }

    void end_frame()
    {
        bgfx::frame();
    }
}
