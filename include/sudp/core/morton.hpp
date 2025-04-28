#pragma once
#include <cstdint>

namespace dmw::core
{
/** Encode (x,y,z) 10 bits chacun → Morton 30 b */
[[nodiscard]]
constexpr uint32_t encode_morton10(uint32_t x, uint32_t y, uint32_t z) noexcept
{
    auto split = [](uint32_t v) {
        v = (v | (v << 16)) & 0x030000FF;
        v = (v | (v <<  8)) & 0x0300F00F;
        v = (v | (v <<  4)) & 0x030C30C3;
        v = (v | (v <<  2)) & 0x09249249;
        return v;
    };
    return (split(x) | (split(y) << 1) | (split(z) << 2));
}

/** Décode Morton 30 b vers (x,y,z) 10 bits */
inline void decode_morton10(uint32_t m,
                            uint32_t& x, uint32_t& y, uint32_t& z) noexcept
{
    auto compact = [](uint32_t v) {
        v &= 0x09249249;
        v = (v ^ (v >>  2)) & 0x030C30C3;
        v = (v ^ (v >>  4)) & 0x0300F00F;
        v = (v ^ (v >>  8)) & 0x030000FF;
        v = (v ^ (v >> 16)) & 0x000003FF;
        return v;
    };
    x = compact(m);
    y = compact(m >> 1);
    z = compact(m >> 2);
}
} // namespace dmw::core
