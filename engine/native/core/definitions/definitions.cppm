module;

#include <cstddef>
#include <type_traits>
#include <cstdint>

export module core.defs;
export import core.version;

static_assert(__cplusplus >=  202207L, "Minimum of C++23 required. Consider upgrading your compiler.");

export namespace draco {
    using i8  = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;

    using uint = unsigned int;
    using u8   = uint8_t;
    using u16  = uint16_t;
    using u32  = uint32_t;
    using u64  = uint64_t;

    using f32 = float;
    using f64 = double;

    using isize = int64_t;
    using usize = std::size_t;

    using rawptr   = void*;
    using uintptr  = uintptr_t;
    using ptrdiff  = ptrdiff_t;

    // UTF-32 type
    using rune = u32;

    template<typename T>
    concept arithmetic = std::is_arithmetic_v<T>;

    template<typename T>
    concept trivial = std::is_trivial_v<T>;

    // Whether the default value of a type is just all-0 bytes.
    // This can most commonly be exploited by using memset for these types instead of loop-construct.
    // Must be explicitly specialized to mark a type as such.
    template <typename T>
    struct is_zero_constructible : std::false_type {};

    template <typename T>
    constexpr bool is_zero_constructible_v = is_zero_constructible<T>::value;

    template <typename T>
    concept zero_constructible = is_zero_constructible_v<T>;
}
