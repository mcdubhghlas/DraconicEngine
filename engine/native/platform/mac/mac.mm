module;

#include <SDL3/SDL.h>
import std;

module platform;

namespace draco::platform {
    NativeWindowFrame get_native_handles(void* sdl_window_ptr) {
        SDL_Window* window = static_cast<SDL_Window*>(sdl_window_ptr);
        NativeWindowFrame frame;
        
        SDL_PropertiesID props = SDL_GetWindowProperties(window);

        // We grab the NSView pointer for the native window handle
        frame.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_VIEW_POINTER, nullptr);
        frame.ndt = nullptr; 

        SDL_GetWindowSize(window, &frame.width, &frame.height);
        frame.valid = (frame.nwh != nullptr);
        return frame;
    }
}