import std;

#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <bx/math.h>

import core.filesystem;

import rendering.rhi;
import rendering.rhi.vertex;

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
        return -1;
    }

    const char* driver = SDL_GetCurrentVideoDriver();
    std::println("Driver: {}", driver ? driver : "Unknown");

    void* nwh = nullptr; // Native window handle
    void* ndt = nullptr; // Native display type

#if defined(__linux__)

    SDL_PropertiesID props = SDL_GetWindowProperties(window);

    if (driver && std::string_view(driver) == "x11")
    {
        ndt = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr); // Get the X11 display pointer
        nwh = (void*)(uintptr_t)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0); // Get the X11 window number and cast it to a pointer
    }
    
#endif

    if (!nwh) {
        std::println("Failed to get native window handle");
        return -1;
    }

    if(!ndt) {
        std::println("Failed to get native display type");
        return -1;
    }

    // Init the RHI with the native window handle and initial size
    if (!draco::rhi::init(ndt, nwh, 1280, 720)) {
        std::println("Failed to initialize RHI");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Geometry data for a triangle to test rendering
    // It includes both positions & colors
    draco::rhi::PosColorVertex triangle[] = {
        { 0.0f, 0.5f, 0.0f, 0xff0000ff }, 
        { 0.5f, -0.5f, 0.0f, 0xff00ff00 },
        { -0.5f, -0.5f, 0.0f, 0xffff0000}
    };

    auto vbh = draco::rhi::create_vertex_buffer(triangle, sizeof(triangle));

    // Load the vertex & fragment shaders
    auto vs_data = draco::filesystem::load_binary("vs_triangle.bin");
    auto fs_data = draco::filesystem::load_binary("fs_triangle.bin");

    // If the path is empty, return an error
    if (vs_data.empty() || fs_data.empty()) {
        std::println("Failed to load shaders");
        std::println("Workin' dir: {}", std::filesystem::current_path().string());
        return -1;
    }

    auto vsh = draco::rhi::create_shader(vs_data.data(), (uint32_t)vs_data.size());
    auto fsh = draco::rhi::create_shader(fs_data.data(), (uint32_t)fs_data.size());
    

    // TODO: Expose our own macros for the state flags instead of using bgfx's directly, tis is just for testin'
    auto pipeline = draco::rhi::create_pipeline({vsh, fsh, (0 
        | BGFX_STATE_WRITE_RGB 
        | BGFX_STATE_WRITE_A 
        | BGFX_STATE_MSAA 
        | BGFX_STATE_PT_TRISTRIP)});

    bool running = true;

    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
                running = false;
        }

        int w, h;
        SDL_GetWindowSize(window, &w, &h);

        draco::rhi::resize(uint16_t(w), uint16_t(h));

        draco::rhi::begin_frame();

        draco::rhi::RenderPacket packet{};
        packet.vertex_buffer = vbh;
        packet.pipeline = pipeline;
        bx::mtxIdentity(packet.model);
        
        draco::rhi::submit(packet, 0);

        draco::rhi::end_frame();
    }

    draco::rhi::shutdown();

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

// Fun fact: AR literally went mad & tis is the result