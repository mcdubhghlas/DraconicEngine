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
        uint64_t state = 0;
    };

    static std::vector<Buffer>   g_buffers;
    static std::vector<Pipeline> g_pipelines;

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

        // Clear everything so we don't have dangling handles
        g_buffers.clear();
        g_pipelines.clear();

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

    ShaderHandle create_shader(const void* data, uint32_t size)
    {
        const bgfx::Memory* mem = bgfx::copy(data, size);
        return bgfx::createShader(mem).idx;
    }

    PipelineHandle create_pipeline(const PipelineDesc& desc)
    {
        bgfx::ShaderHandle vs{ desc.vs };
        bgfx::ShaderHandle fs{ desc.fs };

        bgfx::ProgramHandle prog = bgfx::createProgram(vs, fs, true);

        g_pipelines.push_back({ prog, desc.state });
        return (PipelineHandle)(g_pipelines.size() - 1);
    }

    BufferHandle create_vertex_buffer(const void* data, uint32_t size)
    {
        bgfx::VertexLayout layout;
        layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
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
        bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(
            bgfx::copy(data, size)
        );

        g_buffers.push_back({ BGFX_INVALID_HANDLE, ibh, true });
        return (BufferHandle)(g_buffers.size() - 1);
    }

    void submit(const RenderPacket& p, ViewID view)
    {
        if (p.pipeline >= g_pipelines.size() || p.vertex_buffer >= g_buffers.size()) {
            std::println("Error: Invalid handle in RenderPacket");
            return;
        }
        Pipeline& pipe = g_pipelines[p.pipeline];
        Buffer& vb = g_buffers[p.vertex_buffer];

        bgfx::setTransform(p.model);
        bgfx::setVertexBuffer(0, vb.vbh);

        if (p.index_buffer != UINT16_MAX)
        {
            if (p.index_buffer >= g_buffers.size()) {
                std::println("Error: Invalid index buffer handle in RenderPacket");
                return;
            }
            Buffer& ib = g_buffers[p.index_buffer];
            if (ib.is_index)
                bgfx::setIndexBuffer(ib.ibh);
        }

        bgfx::setState(pipe.state);
        bgfx::submit(view, pipe.program);
    }

    void begin_frame()
    {
        bgfx::setViewRect(0, 0, 0, g_width, g_height);

        float view[16];
        bx::mtxIdentity(view);

        float proj[16];
        bx::mtxOrtho(
            proj,
            -1.0f, 1.0f,
            -1.0f, 1.0f,
            0.0f, 100.0f,
            0.0f,
            false
        );

        bgfx::setViewTransform(0, view, proj);
        bgfx::touch(0);
    }

    void end_frame()
    {
        bgfx::frame();
    }
}