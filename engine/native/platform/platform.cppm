export module platform;

import std;

export namespace draco::platform {
    struct NativeWindowFrame
    {
        void* nwh = nullptr; // Native Window Handle
        void* ndt = nullptr; // Native Display Type

        int width = 0;
        int height = 0;

        bool valid = false;
    };

    NativeWindowFrame get_native_handles(void* sdl_window_ptr);
}