module;

#include <SDL3/SDL.h>

export module input;

import core.stdtypes;

export namespace draco::input
{
    enum class Key : u16
    {
        // TODO: A small set of keys should be okay for now but this needs to be updated later as per our needs
        W, A, S, D,
        Space, Ctrl,
        Shift,
        Escape,
        Invalid
    };

    // Note: This isn't the same as RHI
    void beginFrame();
    void endFrame();

    void setKey(Key key, bool down);
    bool isDown(Key key);

    void processEvent(const SDL_Event& e);

    void setMouseCaptured(SDL_Window* window, bool enabled);
    bool isMouseCaptured();
    
    void setMouseDelta(f32 dx, f32 dy);

    f32 getMouseDx();
    f32 getMouseDy();
}