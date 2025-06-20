#include "net/session.hpp"
#include "core/udp_header.hpp"
#include "core/reassembly_buffer.hpp"

#include <octomap/OcTree.h>
#include <octomap/AbstractOcTree.h>
#include <octomap/ColorOcTree.h> 
#include <zlib.h>
#include <sstream>
#include <iostream>
#include <chrono>
#include "net/add_bt_header.hpp"  

namespace sudp::net {

Session::Session(asio::ip::udp::socket&& sock, DbQueue& dbq, std::size_t mtu)
    : socket_(std::move(sock)), db_queue_(dbq), mtu_(mtu) {
    read_next();
}

void Session::read_next() {
    std::cout.setf(std::ios::unitbuf); // flush stdout after each output
    std::cerr.setf(std::ios::unitbuf); // flush stderr after each output
    recv_buf_.resize(mtu_ + sizeof(core::UdpHdr));
    socket_.async_receive_from(
        asio::buffer(recv_buf_), remote_,
        [this](std::error_code ec, std::size_t n) {
            if (!ec) handle_packet(n);
            read_next();
        });
}


void Session::handle_packet(std::size_t nbytes)
{
    if (nbytes < sizeof(core::UdpHdr))
        return;

    /* ── 1. reconstruction du blob ──────────────────────────────── */
    const auto* hdr   = reinterpret_cast<const core::UdpHdr*>(recv_buf_.data());
    const uint8_t* body = recv_buf_.data() + sizeof(core::UdpHdr);

    auto& buf = buffers_[hdr->seq];
    if (!buf)
        buf = std::make_unique<core::ReassemblyBuffer>(hdr->tot);

    if (!buf->write(hdr->off, body, hdr->len) || !buf->complete())
        return;                                   // pas fini → on attend

    std::vector<uint8_t> blob = std::move(buf->data());
    buffers_.erase(hdr->seq);                     // buffer consommé

    /* ── 2. décompression éventuelle (FLAG_GZIP) ────────────────── */
    if (hdr->flags & core::FLAG_GZIP) {
        if (blob.size() < 4) return;              // 4 oct. = taille brute

        uint32_t raw_len = 0;
        std::memcpy(&raw_len, blob.data(), 4);

        std::vector<uint8_t> out(raw_len);
        uLongf dst = raw_len;
        if (::uncompress(out.data(), &dst,
                         blob.data() + 4, blob.size() - 4) != Z_OK) {
            std::cerr << "[SESSION] zlib-uncompress failed\n";
            return;
        }
        blob.swap(out);
    }

    /* ── 3. lecture directe des données binaires d’occupancy ─────── */
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    ss.write(reinterpret_cast<char*>(blob.data()), blob.size());
    ss.seekg(0);

    constexpr double TREE_RES = 0.05;             // même résol. que le drone
    auto tree = std::make_unique<octomap::ColorOcTree>(TREE_RES);

    if (!tree->readBinaryData(ss)) {              // <-- plus de pré-header
        std::cerr << "[SESSION] Failed to read octree from blob\n";
        return;
    }

    /* ── 4. conversion → nuage de points + push DB queue ─────────── */
    const int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch()).count();

    std::vector<db::PointRGB> pts;
    pts.reserve(tree->size());

    for (auto it = tree->begin_leafs(); it != tree->end_leafs(); ++it) {
        if (!tree->isNodeOccupied(*it)) continue; // garde uniquement les voxels occupés

        db::PointRGB p;
        p.x = it.getX();   p.y = it.getY();   p.z = it.getZ();
        p.r = 128;  p.g = 128; p.b = 128; p.a = 255;
        p.ts = now_ms;
        pts.push_back(p);
    }

    if (!pts.empty()) {
        std::cout << "[SESSION] pushing " << pts.size()
                  << " pts from drone " << int(hdr->drone_id)
                  << " (seq=" << hdr->seq << ")\n";
        db_queue_.push_points(std::move(pts));
    } else {
        std::cout << "[SESSION] Octree contained no occupied voxels\n";
    }
}


} // namespace sudp::net