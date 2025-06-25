#include "net/session.hpp"
#include "core/udp_header.hpp"
#include "core/reassembly_buffer.hpp"
#include "net/drone_manager.hpp"

#include <octomap/OcTree.h>
#include <zlib.h>
#include <sstream>
#include <iostream>
#include <chrono>
#include <cstring>   // std::memcpy
#include <cmath>     // std::llround

namespace sudp::net {

Session::Session(asio::ip::udp::socket&& sock,
                 std::function<void(const std::string&, std::vector<db::PointRGB>&&)> cb,
                 std::size_t mtu)
    : socket_(std::move(sock)),
      mtu_(mtu),
      on_packet_cb_(std::move(cb))
{
    read_next();
}

/* ─────────────────────────────────────────────────────────── */

void Session::read_next()
{
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    recv_buf_.resize(mtu_ + sizeof(core::UdpHdr));

    socket_.async_receive_from(
        asio::buffer(recv_buf_), remote_,
        [this](std::error_code ec, std::size_t n) {
            if (!ec)
                handle_packet(n);
            read_next();                             // on se remet en écoute
        });
}

/* ─────────────────────────────────────────────────────────── */

void Session::handle_packet(std::size_t nbytes)
{
    if (nbytes < sizeof(core::UdpHdr))
        return;

    /* 1. Ré-assemblage des fragments UDP ─────────────────────── */
    const auto* hdr  = reinterpret_cast<const core::UdpHdr*>(recv_buf_.data());
    const uint8_t* body = recv_buf_.data() + sizeof(core::UdpHdr);

    auto& buf = buffers_[hdr->seq];
    if (!buf)
        buf = std::make_unique<core::ReassemblyBuffer>(hdr->tot);

    if (!buf->write(hdr->off, body, hdr->len) || !buf->complete())
        return;                                       // toujours incomplet

    std::vector<uint8_t> blob = std::move(buf->data());
    buffers_.erase(hdr->seq);

    /* 2. Décompression (optionnel) ───────────────────────────── */
    if (hdr->flags & core::FLAG_GZIP) {
        if (blob.size() < 4) return;                  // 4 octets = taille brute

        uint32_t raw_len = 0;
        std::memcpy(&raw_len, blob.data(), sizeof(uint32_t));

        std::vector<uint8_t> out(raw_len);
        uLongf dst_len = raw_len;
        if (::uncompress(out.data(), &dst_len,
                         blob.data() + 4, blob.size() - 4) != Z_OK) {
            std::cerr << "[SESSION] zlib-uncompress failed\n";
            return;
        }
        blob.swap(out);
    }

    /* 3. Flux mémoire vers OctoMap ───────────────────────────── */
    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    ss.write(reinterpret_cast<char*>(blob.data()), blob.size());
    ss.seekg(0);

    /* 4. Lecture de l’arbre ──────────────────────────────────── */
    auto tree = std::make_unique<octomap::OcTree>(0.01); // valeur provisoire
    if (!tree->readBinaryData(ss)) {
        std::cerr << "[SESSION] Failed to read octree from blob\n";
        return;
    }

    const double RES      = tree->getResolution();   // résolution réelle
    const double INV_RES  = 1.0 / RES;

    /* 5. Conversion en nuage de points ───────────────────────── */
    const int64_t now_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

    std::vector<db::PointRGB> pts;
    pts.reserve(tree->size());

    std::cout << "[SESSION] Octree stats: size=" << tree->size()
              << " | leafs=" << tree->getNumLeafNodes()
              << " | res=" << RES << " m\n";

    for (auto it = tree->begin_leafs(); it != tree->end_leafs(); ++it) {
        db::PointRGB p;

        /* quantification à la résolution → indices de voxel */
        p.x = static_cast<int32_t>(std::llround(it.getX() * INV_RES));
        p.y = static_cast<int32_t>(std::llround(it.getY() * INV_RES));
        p.z = static_cast<int32_t>(std::llround(it.getZ() * INV_RES));

        p.r = 128; p.g = 128; p.b = 128; p.a = 255;
        p.ts = now_ms;
        pts.push_back(p);
    }

    std::cout << "[DEBUG-SESSION] Extracted "
              << pts.size() << " points from octree.\n";

    /* 6. Callback vers la base ───────────────────────────────── */
    if (!pts.empty()) {
        std::ostringstream id;
        id << remote_.address().to_string() << ':' << remote_.port();

        if (on_packet_cb_) {
            std::cout << "[SESSION] Inserting " << pts.size()
                      << " points for drone: " << id.str() << '\n';
            on_packet_cb_(id.str(), std::move(pts));
        } else {
            std::cout << "[SESSION] No callback set for drone: "
                      << id.str() << '\n';
        }
    } else {
        std::cout << "[SESSION] No occupied points to insert.\n";
    }
}

}  // namespace sudp::net
