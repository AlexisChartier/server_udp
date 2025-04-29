#pragma once
#include "sudp/core/packet_parser.hpp"
#include "util/lockfree_queue.hpp"
#include <vector>

namespace sudp::net
{
/** Conteneur prêt pour la BD ------------------------------------- */
struct VoxBatch
{
    uint16_t drone_id;
    std::vector<common_dmw::Voxel> vox;
};

template<std::size_t QN = 32768>
using BatchQueue = util::LockfreeQueue<VoxBatch, QN>;

/**
 * Transforme un datagramme → VoxBatch et pousse dans la queue partagée.
 * Pas de mémoire dynamique hors du vector (réservé).
 */
class Session
{
public:
    Session(const uint8_t* data, std::size_t len, BatchQueue<>& q);
};
} // namespace sudp::net
