#pragma once
/**
 *  Format « voxel stream » commun à TOUS les composants :
 *
 *  ┌────────┬───────┬────────┬───────────────┐
 *  │ Offset │ Taille│ Champ  │ Description   │
 *  ├────────┼───────┼────────┼───────────────┤
 *  │   0    │ 1  o  │ ver    │ Version proto │
 *  │   1    │ 1  o  │ flags  │ cf. enum      │
 *  │   2    │ 2  o  │ drone  │ ID drone      │
 *  │   4    │ 2  o  │ count  │ #voxels       │
 *  │   6    │ 2  o  │ resvd  │ 0 (align)     │
 *  └────────┴───────┴────────┴───────────────┘
 *  Puis ‘count’ entrées Voxel (8 o chacune).
 */

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace common_dmw
{
/* --- indicateurs éventuels (compression, ack, etc.) --------------- */
enum PacketFlags : uint8_t
{
    FLAG_NONE   = 0x00,
    // futur : FLAG_COMPRESSED = 0x01,
};

/* ----------- entête alignée sur 1 octet --------------------------- */
#pragma pack(push,1)
struct PacketHeader
{
    uint8_t  ver   {1};
    uint8_t  flags {FLAG_NONE};
    uint16_t drone_id {0};
    uint16_t count {0};
    uint16_t reserved {0};
};
#pragma pack(pop)

static_assert(sizeof(PacketHeader) == 8, "PacketHeader must be 8 bytes");

} // namespace common_dmw
