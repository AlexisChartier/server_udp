#pragma once
#include <cstdint>
namespace dmw::core
{
struct Voxel
{
    uint32_t morton;   ///< Morton-encoded position (10-10-10)
    uint32_t rgb;      ///< 0x00RRGGBB (8 bits par canal)
};
static_assert(sizeof(Voxel) == 8, "Voxel must stay compact (8 bytes)");
} // namespace dmw::core
