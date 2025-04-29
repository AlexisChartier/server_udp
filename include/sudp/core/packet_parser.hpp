#pragma once
/**
 *  Fonctions utilitaires pour analyser un datagramme reçu
 *  (header + tableau de voxels) sans faire de copies.
 */
#include "packet_header.hpp"
#include "voxel.hpp"
#include <span> // Requires C++20
#include <stdexcept>

namespace common_dmw
{
/* Vue non-possédante sur un paquet complet ------------------------ */
struct ParsedPacket
{
    const PacketHeader*        hdr;     ///< ptr vers l'en-tête
    std::span<const Voxel>     voxels;  ///< vue sur le payload
};

/**
 *  Analyse un tampon reçu et renvoie une vue ParsedPacket.
 *  - Ne copie pas les données
 *  - Lève std::runtime_error si format invalide.
 */
inline ParsedPacket parse_packet(const void* data, std::size_t size)
{
    if (size < sizeof(PacketHeader))
        throw std::runtime_error("packet too small");

    auto hdr = reinterpret_cast<const PacketHeader*>(data);

    std::size_t expected = sizeof(PacketHeader)
                         + static_cast<std::size_t>(hdr->count)
                           * sizeof(Voxel);
    if (size < expected)
        throw std::runtime_error("truncated packet (count mismatch)");

    const uint8_t* payload =
        static_cast<const uint8_t*>(data) + sizeof(PacketHeader);

    auto vox_span = std::span<const Voxel>(
        reinterpret_cast<const Voxel*>(payload), static_cast<std::size_t>(hdr->count));

    return {hdr, vox_span};
}

/* Petit helper : taille totale d’un paquet depuis l’entête -------- */
[[nodiscard]]
inline std::size_t packet_size_bytes(const PacketHeader& h) noexcept
{
    return sizeof(PacketHeader)
         + static_cast<std::size_t>(h.count) * sizeof(Voxel);
}

} // namespace common_dmw
