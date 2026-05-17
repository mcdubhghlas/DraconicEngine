#include <print>
#include <cstring>
#include <math.h>

#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <bx/math.h>

import core.io.filesystem;
import core.io.image_loader;

import input;
import platform;
import scene;
import scene.camera.controller;
import scene.transform;
import scene.renderable;

import rendering.rhi;
import rendering.rhi.vertex;
import rendering.rhi.uniform_registry;
import rendering.renderer;
import rendering.mesh;
import rendering.material;
import rendering.quad_renderer;

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

    // Force SDL to pump initial setup events so Wayland can bake the surface
    SDL_SyncWindow(window); 
    for (int i = 0; i < 10; ++i) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            // Just consuming initial configuration events
        }
        SDL_Delay(10);
    }

    draco::input::set_mouse_captured(window, true);

    auto handles = draco::platform::get_native_handles(window);

    if (!handles.valid) {
        std::println("Failed to get native handles");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    if (!draco::rendering::rhi::init(handles.ndt, handles.nwh, handles.type, 1280, 720)) {
        std::println("RHI init failed");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    draco::rendering::renderer::init(1280, 720);

    auto cube_mesh = draco::rendering::mesh::create_cube(); 
    auto plane_mesh = draco::rendering::mesh::create_plane(5.0f); 
    auto sphere_mesh = draco::rendering::mesh::create_sphere(24, 16); 
    auto cylinder_mesh = draco::rendering::mesh::create_cylinder(24, 2.0f); 
    auto capsule_mesh = draco::rendering::mesh::create_capsule(24, 12, 2.0f);

    auto img = draco::core::io::image_loader::load_image("test.png");

    draco::rendering::rhi::TextureHandle tex = draco::rendering::rhi::InvalidTexture;

    if (img.is_valid) {
        tex = draco::rendering::rhi::create_texture(img.pixels.data(), img.width, img.height);
    }

    auto s_texColor = draco::rendering::rhi::create_uniform("s_texColor", draco::rendering::rhi::UniformType::Sampler);

    auto vs = draco::core::io::filesystem::load_binary("vs.bin");
    auto fs = draco::core::io::filesystem::load_binary("fs.bin");

    auto vs_quad = draco::core::io::filesystem::load_binary("vs_quad.bin");
    auto fs_quad = draco::core::io::filesystem::load_binary("fs_quad.bin");

    if (vs.empty() || fs.empty() || vs_quad.empty() || fs_quad.empty()) {
        std::println("Shader load failed");
        draco::rendering::rhi::shutdown();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    auto vsh = draco::rendering::rhi::create_shader(vs.data(), (uint32_t)vs.size());
    auto fsh = draco::rendering::rhi::create_shader(fs.data(), (uint32_t)fs.size());

    auto vsh_quad = draco::rendering::rhi::create_shader(vs_quad.data(), (uint32_t)vs_quad.size());
    auto fsh_quad = draco::rendering::rhi::create_shader(fs_quad.data(), (uint32_t)fs_quad.size());

    auto pipeline = draco::rendering::rhi::create_pipeline({vsh, fsh, draco::rendering::rhi::PipelineState::WriteRGB | draco::rendering::rhi::PipelineState::WriteAlpha | draco::rendering::rhi::PipelineState::MSAA, draco::rendering::rhi::BlendMode::None, draco::rendering::rhi::DepthTest::Less, draco::rendering::rhi::CullMode::CCW, true});

    auto pipeline_quad = draco::rendering::rhi::create_pipeline({vsh_quad, fsh_quad, draco::rendering::rhi::PipelineState::WriteRGB | draco::rendering::rhi::PipelineState::WriteAlpha | draco::rendering::rhi::PipelineState::MSAA, draco::rendering::rhi::BlendMode::Alpha, draco::rendering::rhi::DepthTest::None, draco::rendering::rhi::CullMode::None, true});

    draco::rendering::quad_renderer::QuadRenderer quad_renderer;
    quad_renderer.init(pipeline_quad);

    draco::scene::CameraController camera;
    camera.init();

    auto u_tint   = draco::rendering::rhi::create_uniform("u_tint",   draco::rendering::rhi::UniformType::Vec4);
    auto u_offset = draco::rendering::rhi::create_uniform("u_offset", draco::rendering::rhi::UniformType::Vec4);

    draco::rendering::rhi::register_uniform(draco::rendering::rhi::hash_uniform("u_tint"), u_tint);

    draco::rendering::rhi::register_uniform(draco::rendering::rhi::hash_uniform("u_offset"), u_offset);

    float tint[4]   = {1,1,1,1};
    float offset[4] = {0,0,0,0};

    bool running = true;
    bool mouse_captured = true;

    draco::rendering::material::Material mat{};
    mat.pipeline = pipeline;
    mat.texture = tex;
    mat.sampler = s_texColor;

    mat.uniforms.push_back({.name_hash = draco::rendering::rhi::hash_uniform("u_tint"), .data = tint, .count = 1});

    mat.uniforms.push_back({.name_hash = draco::rendering::rhi::hash_uniform("u_offset"), .data = offset, .count = 1});

    draco::scene::Scene scene;

    scene.renderables.push_back({cube_mesh, draco::scene::transform::make_transform(), mat});
    scene.renderables.push_back({plane_mesh, draco::scene::transform::make_transform(), mat});
    scene.renderables.push_back({sphere_mesh, draco::scene::transform::make_transform(), mat});
    scene.renderables.push_back({cylinder_mesh, draco::scene::transform::make_transform(), mat});
    scene.renderables.push_back({capsule_mesh, draco::scene::transform::make_transform(), mat});

    draco::scene::transform::set_position(scene.renderables[0].transform, -12.0f, 0.0f, 0.0f);
    draco::scene::transform::set_position(scene.renderables[1].transform, -6.0f, 0.0f, 0.0f);
    draco::scene::transform::set_position(scene.renderables[2].transform, 0.0f, 0.0f, 0.0f);
    draco::scene::transform::set_position(scene.renderables[3].transform, 6.0f, 0.0f, 0.0f);
    draco::scene::transform::set_position(scene.renderables[4].transform, 12.0f, 0.0f, 0.0f);

    draco::scene::transform::set_rotation(scene.renderables[1].transform, -bx::kPiHalf, 0.0f, 0.0f);

    while (running)
    {
        static uint64_t last = SDL_GetTicks();
        uint64_t now = SDL_GetTicks();
        float dt = (now - last) / 1000.0f;
        last = now;

        SDL_Event e;
        draco::input::begin_frame();

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
                running = false;

            if (e.type == SDL_EVENT_KEY_DOWN &&
                e.key.key == SDLK_ESCAPE)
            {
                mouse_captured = !mouse_captured;
                draco::input::set_mouse_captured(window, mouse_captured);
            }

            draco::input::process_event(e);
        }

        int w, h;
        SDL_GetWindowSize(window, &w, &h);

        if (w <= 0 || h <= 0)
        {
            continue;
        }

        draco::rendering::rhi::resize((uint16_t)w, (uint16_t)h);
        draco::rendering::renderer::resize((uint16_t)w, (uint16_t)h);

        camera.update(dt);
        auto cam = camera.get_camera();

        draco::rendering::renderer::begin_frame(cam);
        draco::rendering::renderer::render_scene(scene);

        quad_renderer.begin();

        static float quad_base_x = 400.0f;
        static float quad_base_y = 300.0f;

        for (int i = 0; i < 50; i++)
        {
            draco::rendering::quad_renderer::QuadCommand q{};

            q.texture = tex;
            q.color = 0xffffffff;
            q.x = quad_base_x + std::sin(SDL_GetTicks() * 0.001f + i) * 50.0f;
            q.y = quad_base_y + i * 6.0f;
            q.width = 50.0f;
            q.height = 50.0f;
            q.rotation = SDL_GetTicks() * 0.001f;

            quad_renderer.submit(q);
        }

        draco::rendering::renderer::submit_ui(quad_renderer);

        draco::rendering::renderer::end_frame();
    }

    draco::rendering::rhi::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
