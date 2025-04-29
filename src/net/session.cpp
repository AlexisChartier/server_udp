#include "net/session.hpp"

namespace sudp::net

{

Session::Session(const uint8_t* data, std::size_t len, BatchQueue<>& q)

{

    auto pkt = common_dmw::parse_packet(data, len);

    VoxBatch batch;

    batch.drone_id = pkt.hdr->drone_id;

    batch.vox.assign(pkt.voxels.begin(), pkt.voxels.end());

    q.push(batch);  // ignore si queue pleine

}

} // namespace sudp::net