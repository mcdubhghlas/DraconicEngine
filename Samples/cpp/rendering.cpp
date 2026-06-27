#include <print>
#include <cstring>
#include <math.h>

#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <bx/math.h>

import draconic;

int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println("SDL init failed: {}", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Draconic Engine Rendering Sample",
        1280, 720,
        SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::println("Failed to create window: {}", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    draco::input::setMouseCaptured(window, true);

    auto handles = draco::platform::getNativeHandles(window);

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

    auto cube_mesh = draco::rendering::mesh::createCube();
    auto plane_mesh = draco::rendering::mesh::createPlane(5.0f);
    auto sphere_mesh = draco::rendering::mesh::createSphere(24, 16);
    auto cylinder_mesh = draco::rendering::mesh::createCylinder(24, 2.0f);
    auto capsule_mesh = draco::rendering::mesh::createCapsule(24, 12, 2.0f);

    auto img = draco::core::io::loader::image::loadImage("test.png");

    draco::rendering::rhi::TextureHandle tex = draco::rendering::rhi::InvalidTexture;

    if (img.isValid) {
        tex = draco::rendering::rhi::createTexture(img.pixels.data(), img.width, img.height);
    }

    auto s_texColor = draco::rendering::rhi::createUniform("s_texColor", draco::rendering::rhi::UniformType::Sampler);

    auto vs = draco::core::io::filesystem::loadBinary("vs.bin");
    auto fs = draco::core::io::filesystem::loadBinary("fs.bin");

    auto vs_quad = draco::core::io::filesystem::loadBinary("vs_quad.bin");
    auto fs_quad = draco::core::io::filesystem::loadBinary("fs_quad.bin");

    if (vs.empty() || fs.empty() || vs_quad.empty() || fs_quad.empty()) {
        std::println("Shader load failed");
        draco::rendering::rhi::shutdown();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    auto vsh = draco::rendering::rhi::createShader(vs.data(), (draco::u32)vs.size());
    auto fsh = draco::rendering::rhi::createShader(fs.data(), (draco::u32)fs.size());

    auto vsh_quad = draco::rendering::rhi::createShader(vs_quad.data(), (draco::u32)vs_quad.size());
    auto fsh_quad = draco::rendering::rhi::createShader(fs_quad.data(), (draco::u32)fs_quad.size());

    auto pipeline = draco::rendering::rhi::createPipeline({vsh, fsh, draco::rendering::rhi::PipelineState::WriteRGB | draco::rendering::rhi::PipelineState::WriteAlpha | draco::rendering::rhi::PipelineState::MSAA, draco::rendering::rhi::BlendMode::None, draco::rendering::rhi::DepthTest::Less, draco::rendering::rhi::CullMode::CCW, true});

    auto pipeline_quad = draco::rendering::rhi::createPipeline({vsh_quad, fsh_quad, draco::rendering::rhi::PipelineState::WriteRGB | draco::rendering::rhi::PipelineState::WriteAlpha | draco::rendering::rhi::PipelineState::MSAA, draco::rendering::rhi::BlendMode::Alpha, draco::rendering::rhi::DepthTest::None, draco::rendering::rhi::CullMode::None, true});

    draco::rendering::quad::QuadRenderer quad_renderer;
    quad_renderer.init(pipeline_quad);

    draco::scene::CameraController camera;
    camera.init();

    auto u_tint   = draco::rendering::rhi::createUniform("u_tint",   draco::rendering::rhi::UniformType::Vec4);
    auto u_offset = draco::rendering::rhi::createUniform("u_offset", draco::rendering::rhi::UniformType::Vec4);

    draco::rendering::rhi::registerUniform(draco::rendering::rhi::hashUniform("u_tint"), u_tint);

    draco::rendering::rhi::registerUniform(draco::rendering::rhi::hashUniform("u_offset"), u_offset);

    draco::f32 tint[4]   = {1,1,1,1};
    draco::f32 offset[4] = {0,0,0,0};

    bool running = true;
    bool mouse_captured = true;

    draco::rendering::material::Material mat{};
    mat.pipeline = pipeline;
    mat.texture = tex;
    mat.sampler = s_texColor;

    mat.uniforms.push_back({.nameHash = draco::rendering::rhi::hashUniform("u_tint"), .data = tint, .count = 1});
    mat.uniforms.push_back({.nameHash = draco::rendering::rhi::hashUniform("u_offset"), .data = offset, .count = 1});

    draco::scene::Scene scene;

    static constexpr draco::math::Transform tr;

    scene.renderables.push_back({cube_mesh, tr, mat});
    scene.renderables.push_back({plane_mesh, tr, mat});
    scene.renderables.push_back({sphere_mesh, tr, mat});
    scene.renderables.push_back({cylinder_mesh, tr, mat});
    scene.renderables.push_back({capsule_mesh, tr, mat});

    scene.renderables[0].transform.setPosition(-12.0f, 0.0f, 0.0f);
    scene.renderables[1].transform.setPosition(-6.0f, 0.0f, 0.0f);
    scene.renderables[2].transform.setPosition(0.0f, 0.0f, 0.0f);
    scene.renderables[3].transform.setPosition(6.0f, 0.0f, 0.0f);
    scene.renderables[4].transform.setPosition(12.0f, 0.0f, 0.0f);

    scene.renderables[1].transform.setRotation(-bx::kPiHalf, 0.0f, 0.0f);
    
    while (running)
    {
        static draco::u64 last = SDL_GetTicks();
        draco::u64 now = SDL_GetTicks();
        draco::f32 dt = static_cast<draco::f32>(now - last) / 1000.0f;
        last = now;

        SDL_Event e;
        draco::input::beginFrame();

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
                running = false;

            if (e.type == SDL_EVENT_KEY_DOWN &&
                e.key.key == SDLK_ESCAPE)
            {
                mouse_captured = !mouse_captured;
                draco::input::setMouseCaptured(window, mouse_captured);
            }

            draco::input::processEvent(e);
        }

        int w, h;
        SDL_GetWindowSize(window, &w, &h);

        if (w <= 0 || h <= 0)
        {
            continue;
        }

        draco::rendering::rhi::resize((draco::u16)w, (draco::u16)h);
        draco::rendering::renderer::resize((draco::u16)w, (draco::u16)h);

        camera.update(dt);
        auto cam = camera.getCamera();

        draco::rendering::renderer::beginFrame(cam);
        
        for (const auto& renderable : scene.renderables)
        {
            draco::rendering::renderer::submitRenderable(renderable.transform,  renderable.material,  renderable.mesh);
        }

        quad_renderer.begin();

        static draco::f32 quad_base_x = 400.0f;
        static draco::f32 quad_base_y = 300.0f;

        for (int i = 0; i < 50; i++)
        {
            draco::rendering::quad::QuadCommand q{};

            q.texture = tex;
            q.color = 0xffffffff;
            q.x = quad_base_x + std::sin(SDL_GetTicks() * 0.001f + i) * 50.0f;
            q.y = quad_base_y + i * 6.0f;
            q.width = 50.0f;
            q.height = 50.0f;
            q.rotation = SDL_GetTicks() * 0.001f;

            quad_renderer.submit(q);
        }

        draco::rendering::renderer::submitUI(quad_renderer);

        draco::rendering::renderer::endFrame();
    }

    draco::rendering::rhi::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
