module;

#include <cstdint>

export module rendering.rhi.vertex;

import std;

export namespace draco::rendering::rhi {
    enum class Attrib { 
        Position, 
        Color0, 
        TexCoord0, 
        Normal, 
        Tangent 
    };

    enum class AttribType { 
        Float, 
        Uint8 
    };

    struct VertexElement {
        Attrib attrib;
        uint16_t count;
        AttribType type;
        bool normalized = false;
    };

    struct VertexLayoutDesc {
        std::vector<VertexElement> elements;
    };

    struct TexturedVertex {
        float x, y, z;
        uint32_t abgr;
        float u, v;
    };

    // Helper to get the standard layout for the current vertex struct
    inline VertexLayoutDesc get_textured_vertex_layout() {
        return {
            .elements = {
                { Attrib::Position,  3, AttribType::Float },
                { Attrib::Color0,    4, AttribType::Uint8, true },
                { Attrib::TexCoord0, 2, AttribType::Float }
            }
        };
    }
}