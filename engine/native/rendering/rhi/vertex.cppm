module;
#include <bgfx/bgfx.h>

export module rendering.rhi.vertex;

import std;


export namespace draco::rhi
{
    struct PosColorVertex
    {
        float x, y, z; // Position
        uint32_t agbr; // Color, stored as AGBR
    };
}