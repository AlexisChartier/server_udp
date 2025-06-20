#pragma once
#include <libpq-fe.h>
#include <vector>
#include <cstdint>
#include <string>
#include <sstream>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <tuple>

namespace std {
    template<>
    struct hash<std::tuple<int,int,int>>
    {
        std::size_t operator()(const std::tuple<int,int,int>& t) const noexcept
        {
            std::size_t h1 = std::hash<int>{}(std::get<0>(t));
            std::size_t h2 = std::hash<int>{}(std::get<1>(t));
            std::size_t h3 = std::hash<int>{}(std::get<2>(t));
            return h1 ^ (h2<<1) ^ (h3<<2);
        }
    };
}

namespace sudp::db
{
// ───────────────────────────────────────────────────────────────
// données
// ───────────────────────────────────────────────────────────────
struct PointRGB {
    float    x,y,z;
    uint8_t  r,g,b,a;
    int      nb_records = 1;
    int64_t  ts;
};

// ───────────────────────────────────────────────────────────────
// pipeline
// ───────────────────────────────────────────────────────────────
class SpatialPipeline {
public:
    SpatialPipeline(PGconn* conn, std::size_t batch = 10'000)
        : conn_{conn}, batch_size_{batch} {}

    void push(PointRGB&& p)
    {
        rows_.emplace_back(std::move(p));
        if (rows_.size() >= batch_size_)
            flush();
    }

    ~SpatialPipeline() { flush(); }

    /* flush explicit (appelé depuis le worker) */
    void flush_pending() { flush(); }

private:
    void flush()
    {
        if (rows_.empty()) return;

        auto t0 = std::chrono::high_resolution_clock::now();
        std::cout << "[DB-Spatial-pip] Flushing " << rows_.size() << " raw points\n";

        /* 1) fusion des doublons (x,y,z identiques) */
        std::unordered_map<std::tuple<int,int,int>,PointRGB> fused;
        for (const auto& p : rows_) {
            auto key = std::make_tuple(int(p.x), int(p.y), int(p.z));
            auto it  = fused.find(key);
            if (it == fused.end()) fused[key] = p;
            else                   it->second.nb_records += 1;
        }

        std::cout << "[DB-Spatial-pip] Fused " << fused.size() << " unique points\n";

        /* 2) construction de la requête */
        std::ostringstream oss;
        oss << "INSERT INTO spatial_points "
            << "(x,y,z,color_r,color_g,color_b,color_a,timestamp,nb_records) VALUES ";

        std::size_t cnt = 0;
        for (const auto& [key,p] : fused) {
            const auto& [x,y,z] = key;
            oss << "("<<x<<","<<y<<","<<z<<","
                << int(p.r)<<","<<int(p.g)<<","<<int(p.b)<<","<<int(p.a)<<","
                << p.ts<<","<<p.nb_records<<")";
            if (++cnt < fused.size()) oss << ',';
        }
        oss << " ON CONFLICT (x,y,z) "
            << "DO UPDATE SET nb_records = spatial_points.nb_records + EXCLUDED.nb_records";

        /* 3) exécution */
        const std::string sql = oss.str();
        PGresult* res = PQexec(conn_, sql.c_str());
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::cerr << "[DB] Bulk insert failed: "
                      << PQerrorMessage(conn_);

            /* tentative de ré-initialisation si la connexion est cassée */
            if (PQstatus(conn_) == CONNECTION_BAD) {
                std::cerr << "[DB] resetting connection …\n";
                PQreset(conn_);
                if (PQstatus(conn_) != CONNECTION_OK)
                    std::cerr << "[DB] reset failed!\n";
            }
        } else {
            std::cout << "[DB] Bulk insert succeeded\n";
        }
        PQclear(res);
        rows_.clear();

        auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::high_resolution_clock::now() - t0).count();
        std::cout << "[DB-Spatial-pip] Flush took " << dt << " ms\n";
    }

    PGconn*                conn_;
    std::size_t            batch_size_;
    std::vector<PointRGB>  rows_;
};

} // namespace sudp::db
