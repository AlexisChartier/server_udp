#include "net/udp_server.hpp"
#include <iostream>

namespace sudp::net {

void UdpServer::start_receive()
{
    socket_.async_receive_from(
        asio::buffer(rx_buf_), remote_,
        [this](auto ec, std::size_t len)
        {
            if (!ec && len >= core::UDP_HDR_SIZE) {        // ← compilera
                pool_.post([this, len]{
                    handle_packet(rx_buf_.data(), len);
                });
            } else {
                std::cerr << "[UDP] Invalid packet: " << len << " bytes\n";
            }
            start_receive();
        });
}

void UdpServer::handle_packet(const uint8_t* data, std::size_t len)
{
    auto* hdr = reinterpret_cast<const core::UdpHdr*>(data);
    const uint8_t* payload   = data + core::UDP_HDR_SIZE;
    std::size_t    pay_len   = len  - core::UDP_HDR_SIZE;

    FragKey key{hdr->drone_id, hdr->seq};

    std::lock_guard lock(mutex_);               // protège buffers_
    auto it = buffers_.find(key);
    if (it == buffers_.end()) {                 // nouvelle séquence
        auto [new_it, _] =
            buffers_.emplace(key, core::ReassemblyBuffer(hdr->tot));
        it = new_it;
        std::cout << "[INFO] new blob drone=" << int(hdr->drone_id)
                  << " seq=" << hdr->seq << " tot=" << hdr->tot << "\n";
    }

    auto& buffer = it->second;
    if (!buffer.write(hdr->off, payload, pay_len)) {
        std::cerr << "[WARN] overlap / bad offset (" << hdr->off << ")\n";
        return;
    }

    if (buffer.complete()) {
        std::cout << "[INFO] blob complete, pushing to DB queue\n";
        queue_.push(DbItem{
            .drone_id = hdr->drone_id,
            .seq      = hdr->seq,
            .flags    = hdr->flags,
            .payload  = std::move(buffer.data())
        });
        buffers_.erase(it);
    }
}

} // namespace sudp::net
