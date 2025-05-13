#include "net/session.hpp"
#include <iostream>
namespace sudp::net

{

    Session::Session(const uint8_t* data, std::size_t len, BatchQueue<>& q)

    {
    
        try {
    
            auto pkt = common_dmw::parse_packet(data, len);
    
    
            VoxBatch batch;
    
            batch.drone_id = pkt.hdr->drone_id;
    
            batch.vox.assign(pkt.voxels.begin(), pkt.voxels.end());
    
    
            std::cout << "[SESSION] drone_id=" << static_cast<int>(batch.drone_id)
    
                      << " | voxels=" << batch.vox.size() << std::endl;
    
    
            if (!q.push(batch)){
    
                std::cerr << "[SESSION] Queue pleine : batch non inséré" << std::endl;
            }
    
        } catch (const std::exception& e) {
            std::cerr << "[SESSION] Erreur lors du parsing: " << e.what() << std::endl;
        }
    
    }

} // namespace sudp::net