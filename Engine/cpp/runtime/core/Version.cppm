module;

#include <format>

export module core.version;
import core.stdtypes;

export namespace draco {

struct Version {
  u16 major = 0;
  u16 minor = 0;
  u16 patch = 0;
};

constexpr Version VERSION{.major = 2026, .minor = 0, .patch = 0};
} // namespace draco

export namespace std {
template <> struct formatter<draco::Version> {
  constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin(); // Accept any format spec (or parse custom ones)
  }

  auto format(const draco::Version &v, std::format_context &ctx) const {
    return std::format_to(ctx.out(), "{}.{}.{}", v.major, v.minor, v.patch);
  }
};
} // namespace std
