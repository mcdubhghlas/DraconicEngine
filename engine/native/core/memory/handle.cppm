module;

#include <cstdint>

export module core.memory.handle;

export namespace draco::core::memory
{
    // Packed generation handle:
    // [ 16 bits generation | 16 bits index ]
    template<typename Tag>
    struct Handle
    {
        uint32_t value = UINT32_MAX;

        static constexpr uint32_t INVALID = UINT32_MAX;

        constexpr Handle() = default;
        constexpr explicit Handle(uint32_t v) : value(v) {}

        constexpr uint16_t index() const
        {
            return static_cast<uint16_t>(value & 0xFFFF);
        }

        constexpr uint16_t generation() const
        {
            return static_cast<uint16_t>(value >> 16);
        }

        constexpr explicit operator bool() const
        {
            return value != INVALID;
        }

        constexpr bool operator==(const Handle& other) const
        {
            return value == other.value;
        }

        constexpr bool operator!=(const Handle& other) const
        {
            return value != other.value;
        }

        static constexpr Handle invalid()
        {
            return Handle{ INVALID };
        }

        static constexpr Handle make(uint16_t index, uint16_t generation)
        {
            return Handle{
                (static_cast<uint32_t>(generation) << 16) |
                static_cast<uint32_t>(index)
            };
        }
    };
}
