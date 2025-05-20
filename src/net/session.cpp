#include "net/session.hpp"
#include "core/udp_header.hpp"
#include "core/reassembly_buffer.hpp"

#include <octomap/OcTree.h>
#include <zlib.h>
#include <sstream>
#include <iostream>
#include <chrono>

namespace sudp::net {

Session::Session(asio::ip::udp::socket&& sock, DbQueue& dbq, std::size_t mtu)
    : socket_(std::move(sock)), db_queue_(dbq), mtu_(mtu) {
    read_next();
}

void Session::read_next() {
    recv_buf_.resize(mtu_ + sizeof(core::UdpHdr));
    socket_.async_receive_from(
        asio::buffer(recv_buf_), remote_,
        [this](std::error_code ec, std::size_t n) {
            if (!ec) handle_packet(n);
            read_next();
        });
}

void Session::handle_packet(std::size_t nbytes) {
    if (nbytes < sizeof(core::UdpHdr)) return;

    const auto* hdr = reinterpret_cast<const core::UdpHdr*>(recv_buf_.data());
    const uint8_t* body = recv_buf_.data() + sizeof(core::UdpHdr);

    auto& buf = buffers_[hdr->seq];
    if (!buf) buf = std::make_unique<core::ReassemblyBuffer>(hdr->tot);

    if (!buf->write(hdr->off, body, hdr->len)) return;
    if (!buf->complete()) return;

    std::vector<uint8_t> blob = std::move(buf->data());
    buffers_.erase(hdr->seq);

    // Décompression si nécessaire
    if (hdr->flags & core::FLAG_GZIP) {
        if (blob.size() < 4) return;
        uint32_t raw = *reinterpret_cast<uint32_t*>(blob.data());
        std::vector<uint8_t> out(raw);
        uLongf dst = raw;
        if (uncompress(out.data(), &dst, blob.data() + 4, blob.size() - 4) != Z_OK)
            return;
        blob.swap(out);
    }

    // Lecture de l’OctoMap
    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    ss.write(reinterpret_cast<char*>(blob.data()), blob.size());
    ss.seekg(0);

    std::unique_ptr<octomap::AbstractOcTree> at{octomap::AbstractOcTree::read(ss)};
    auto* tree = dynamic_cast<octomap::OcTree*>(at.get());
    if (!tree) return;

    const int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::vector<db::PointRGB> pts;
    pts.reserve(tree->size());

    for (auto it = tree->begin_leafs(); it != tree->end_leafs(); ++it) {
        if (!tree->isNodeOccupied(*it)) continue;
        db::PointRGB p;
        p.x = it.getX(); p.y = it.getY(); p.z = it.getZ();
        p.r = 128; p.g = 128; p.b = 128; p.a = 255;
        p.ts = now_ms;
        pts.push_back(p);
    }

    if (!pts.empty())
        db_queue_.push_points(std::move(pts));
}

} // namespace sudp::net