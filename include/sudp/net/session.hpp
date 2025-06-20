#pragma once
#include <asio.hpp>
#include <unordered_map>
#include <memory>
#include <vector>

#include "core/udp_header.hpp"
#include "core/reassembly_buffer.hpp"
#include "db/spatial_pipeline.hpp"

namespace sudp::net {

class Session {
public:
    Session(asio::ip::udp::socket&& sock, std::function<void(const std::string&, std::vector<db::PointRGB>&&)> cb, std::size_t mtu = 1300);

private:
    void read_next();
    void handle_packet(std::size_t nbytes);
    std::function<void(const std::string&, std::vector<db::PointRGB>&&)> on_packet_cb_;
    asio::ip::udp::socket            socket_;
    asio::ip::udp::endpoint          remote_;
    std::vector<uint8_t>             recv_buf_;

    std::size_t                      mtu_;

    using BufPtr = std::unique_ptr<core::ReassemblyBuffer>;
    std::unordered_map<uint32_t, BufPtr> buffers_;  // seq → buffer
};

} // namespace sudp::net
