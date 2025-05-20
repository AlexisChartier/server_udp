#include "net/udp_server.hpp"
#include <iostream>
#include <utility>

namespace sudp::net {

UdpServer::UdpServer(uint16_t port, DbQueue& queue, std::size_t n_workers)
    : socket_{io_, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)}
    , queue_{queue}
    , pool_{n_workers}
{
    std::cout << "[INFO] UDP server initialized on port " << port << "\n"<< std::endl;
    start_receive();
}

void UdpServer::run() {
    io_.run();
}

void UdpServer::start_receive() {
    socket_.async_receive_from(
        asio::buffer(rx_buf_), remote_,
        [this](auto ec, std::size_t len) {
            if (!ec && len >= core::UDP_HDR_SIZE) {
                pool_.post([this, len] {
                    handle_packet(rx_buf_.data(), len);
                });
            } else {
                std::cerr << "[UDP] Invalid packet or short data: " << len << " bytes\n";
            }
            start_receive();
        });
}

void UdpServer::handle_packet(const uint8_t* data, std::size_t len) {
    auto* hdr = reinterpret_cast<const core::UdpHdr*>(data);
    const uint8_t* payload = data + core::UDP_HDR_SIZE;
    std::size_t payload_len = len - core::UDP_HDR_SIZE;

    auto key = FragKey{hdr->drone_id, hdr->seq};
    auto it = buffers_.find(key);
    if (it == buffers_.end()) {
        std::cout << "[INFO] New fragment from drone "
                  << static_cast<int>(hdr->drone_id)
                  << " (seq=" << hdr->seq << ")\n";
        buffers_.emplace(key, core::ReassemblyBuffer(hdr->tot));
    }
    std::lock_guard lock(mutex_);
    auto& buffer = it->second;
    if (buffer.total() == 0) {
        buffer = core::ReassemblyBuffer(hdr->tot);
    }

    if (!buffer.write(hdr->off, payload, payload_len)) {
        std::cerr << "[WARN] Failed to write fragment at offset " << hdr->off << "\n";
        return;
    }

    if (buffer.complete()) {
        std::cout << "[INFO] Reassembled complete blob from drone "
                  << static_cast<int>(hdr->drone_id)
                  << " (seq=" << hdr->seq << ")\n";

        queue_.push(DbItem{
            .drone_id = hdr->drone_id,
            .seq      = hdr->seq,
            .flags    = hdr->flags,
            .payload  = std::move(buffer.data())
        });

        buffers_.erase(key);
    }
}

} // namespace sudp::net

