module;

#include <SDL3/SDL.h>

module input;

import core.stdtypes;

namespace draco::input
{
    // Static globals
    static bool g_keys[256]{};
    static bool g_mouse_captured;
    static f32 g_mouse_dx = 0;
    static f32 g_mouse_dy = 0;

    static Key map_sdl_key(SDL_Keycode k)
    {
        switch (k)
        {
            case SDLK_W: return Key::W;
            case SDLK_A: return Key::A;
            case SDLK_S: return Key::S;
            case SDLK_D: return Key::D;
            case SDLK_SPACE: return Key::Space;
            case SDLK_LCTRL: return Key::Ctrl;
            case SDLK_LSHIFT: return Key::Shift;
            case SDLK_ESCAPE: return Key::Escape;
            default: return Key::Invalid;
        }
    }

    void beginFrame()
    {
        g_mouse_dx = 0;
        g_mouse_dy = 0;
    }

    void endFrame()
    {
    }

    void setKey(Key key, bool down)
    {
        if (key == Key::Invalid) return;

        g_keys[(u16)key] = down;
    }

    bool isDown(Key key)
    {
        return g_keys[(u16)key];
    }

    void processEvent(const SDL_Event& e)
    {
        switch (e.type)
        {
            case SDL_EVENT_KEY_DOWN:
                setKey(map_sdl_key(e.key.key), true);
                break;

            case SDL_EVENT_KEY_UP:
                setKey(map_sdl_key(e.key.key), false);
                break;

            case SDL_EVENT_MOUSE_MOTION:
                setMouseDelta((f32)e.motion.xrel, (f32)e.motion.yrel);
                break;
        }
    }

    void setMouseCaptured(SDL_Window* window, bool enabled)
    {
        g_mouse_captured = enabled;

        SDL_SetWindowRelativeMouseMode(window, enabled);
        SDL_SetWindowMouseGrab(window, enabled);
    }

    bool isMouseCaptured()
    {
        return g_mouse_captured;
    }

    void setMouseDelta(f32 dx, f32 dy)
    {
        g_mouse_dx += dx;
        g_mouse_dy += dy;
    }

    f32 getMouseDx() {
        return g_mouse_dx; 
    }

    f32 getMouseDy() {
        return g_mouse_dy; 
    }
}