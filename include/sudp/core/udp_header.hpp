#pragma once
#include <cstdint>

namespace sudp::core {

#pragma pack(push, 1)
struct UdpHdr          // 16 octets
{
    uint32_t seq;      ///< identifiant du scan complet
    uint32_t tot;      ///< taille totale du blob (octets)
    uint32_t off;      ///< offset du fragment dans le blob
    uint16_t len;      ///< longueur du fragment
    uint8_t  flags;    ///< 0x01=snapshot, 0x02=delta, 0x04=gzip
    uint8_t  drone_id; ///< 0‑255
};
#pragma pack(pop)

inline constexpr std::size_t UDP_HDR_SIZE = sizeof(UdpHdr);
inline constexpr uint8_t FLAG_SNAPSHOT = 0x01;
inline constexpr uint8_t FLAG_DELTA    = 0x02;
inline constexpr uint8_t FLAG_GZIP     = 0x04;

} // namespace sudp::core

