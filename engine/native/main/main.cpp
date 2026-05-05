import std;

#include <SDL3/SDL.h>
#include <bx/math.h>

import core.io.filesystem;
import core.io.image_loader;

import platform;

import rendering.rhi;
import rendering.rhi.vertex;
import rendering.renderer;

int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println("SDL init failed: {}", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Draconic Engine",
        1280, 720,
        SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::println("Failed to create window: {}", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    auto handles = draco::platform::get_native_handles(window);

    if (!handles.valid) {
        std::println("Failed to get native handles");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Init the RHI & Renderer
    if (!draco::rendering::rhi::init(handles.ndt, handles.nwh, 1280, 720)) {
        std::println("RHI init failed");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    draco::rendering::renderer::init(1280, 720);

    draco::rhi::TexturedVertex quad[] = {
        { -0.5f,  0.5f,  0.1f, 0xffffffff, 0.0f, 0.0f },
        {  0.5f,  0.5f, -0.1f, 0xffffffff, 1.0f, 0.0f },
        {  0.5f, -0.5f,  0.1f, 0xffffffff, 1.0f, 1.0f },
        { -0.5f, -0.5f, -0.1f, 0xffffffff, 0.0f, 1.0f }
    };

    uint16_t indices[] = { 0,1,2, 2,3,0 };

    auto vbh = draco::rendering::rhi::create_vertex_buffer(quad, sizeof(quad));
    auto ibh = draco::rendering::rhi::create_index_buffer(indices, sizeof(indices));

    auto img = draco::core::io::image_loader::load_image("test.png");

    draco::rendering::rhi::TextureHandle tex = draco::rendering::rhi::InvalidTexture;
    if (img.is_valid) {
        tex = draco::rendering::rhi::create_texture(img.pixels.data(), img.width, img.height);
    }

    auto s_texColor = draco::rendering::rhi::create_uniform("s_texColor", draco::rendering::rhi::UniformType::Sampler);

    auto vs = draco::core::io::filesystem::load_binary("vs_triangle.bin");
    auto fs = draco::core::io::filesystem::load_binary("fs_triangle.bin");

    if (vs.empty() || fs.empty()) {
        std::println("Shader load failed");
        draco::rendering::rhi::shutdown();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    auto vsh = draco::rendering::rhi::create_shader(vs.data(), (uint32_t)vs.size());
    auto fsh = draco::rendering::rhi::create_shader(fs.data(), (uint32_t)fs.size());

    auto pipeline = draco::rendering::rhi::create_pipeline({
        vsh,
        fsh,
        draco::rendering::rhi::PipelineState::WriteRGB |
        draco::rendering::rhi::PipelineState::WriteAlpha |
        draco::rendering::rhi::PipelineState::MSAA
    });

    auto u_tint   = draco::rendering::rhi::create_uniform("u_tint",   draco::rendering::rhi::UniformType::Vec4);
    auto u_offset = draco::rendering::rhi::create_uniform("u_offset", draco::rendering::rhi::UniformType::Vec4);

    float tint[4]   = {1,1,1,1};
    float offset[4] = {0,0,0,0};

    bool running = true;

    float angle = 0.0f;

    while (running)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT)
                running = false;
        }

        int w, h;
        SDL_GetWindowSize(window, &w, &h);

        draco::rendering::rhi::resize((uint16_t)w, (uint16_t)h);
        draco::rendering::renderer::resize((uint16_t)w, (uint16_t)h);

        draco::rendering::renderer::Camera cam;

        cam.position = { 0.0f, 0.0f, -2.0f }; // camera pulled back
        cam.target   = { 0.0f, 0.0f,  0.0f }; // looking at origin
        cam.up       = { 0.0f, 1.0f,  0.0f };

        cam.fov = 60.0f;
        cam.near_plane = 0.1f;
        cam.far_plane  = 100.0f;

        draco::rendering::renderer::begin_frame(cam);

        draco::rendering::rhi::RenderPacket pkt;
        pkt.vertex_buffer   = vbh;
        pkt.index_buffer    = ibh;
        pkt.pipeline        = pipeline;
        pkt.texture_handle  = tex;
        pkt.texture_unit    = 0;
        pkt.sampler_uniform = s_texColor;

        pkt.uniforms.push_back({ u_tint, tint, 1 });
        pkt.uniforms.push_back({ u_offset, offset, 1 });

        angle += 0.01f;

        float model[16];
        bx::mtxRotateY(model, angle);

        std::memcpy(pkt.model, model, sizeof(model));

        draco::rendering::renderer::submit_entity(pkt, 0);

        for (int i = 0; i < 10; i++)
    {
        auto p = pkt;

        float model[16];
        bx::mtxTranslate(model, i * 0.6f - 3.0f, 0.0f, 0.0f);

        std::memcpy(p.model, model, sizeof(model));

        draco::rendering::renderer::submit_entity(p, 0);
    }

        draco::rendering::renderer::end_frame();
    }

    draco::rendering::rhi::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}