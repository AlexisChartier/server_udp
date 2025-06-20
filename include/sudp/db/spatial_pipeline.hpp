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

    float x, y, z;
    uint8_t r, g, b, a;
    int nb_records = 1;
    int64_t ts;
};

// ───────────────────────────────────────────────────────────────
// pipeline
// ───────────────────────────────────────────────────────────────
class SpatialPipeline {
public:

    SpatialPipeline(PGconn* conn, std::size_t batch = 10000)
        : conn_(conn), batch_size_(batch) {}

    void push(PointRGB&& p)
    {
        rows_.emplace_back(std::move(p));
        if (rows_.size() >= batch_size_)
            flush();
    }

    ~SpatialPipeline() {
        flush();
    }
    void flush_pending() {
            flush();
    }

private:
    void flush()
    {
        if (rows_.empty()) return;
        
        auto start = std::chrono::high_resolution_clock::now();
        std::cout << "[DB-Spatial-pip] Flushing " << rows_.size() << " raw points\n";
      
        // Étape 1 : fusionner les points identiques (x,y,z)
        std::unordered_map<std::tuple<int, int, int>, PointRGB> fused;
        for (const auto& p : rows_) {
            auto key = std::make_tuple(
                static_cast<int>(p.x),
                static_cast<int>(p.y),
                static_cast<int>(p.z)
            );

            auto it = fused.find(key);
            if (it == fused.end()) {
                fused[key] = p;
            } else {
                it->second.nb_records += 1;
            }
        }

        std::cout << "[DB-Spatial-pip] Fused " << fused.size() << " unique points\n";
        // Étape 2 : construire la requête SQL
        std::ostringstream query;
        query << "INSERT INTO spatial_points "
              << "(x, y, z, color_r, color_g, color_b, color_a, timestamp, nb_records) VALUES ";

        std::size_t count = 0;
        for (const auto& [key, p] : fused) {
            const auto& [x, y, z] = key;
            query << "("
                  << x << "," << y << "," << z << ","
                  << static_cast<int>(p.r) << "," << static_cast<int>(p.g) << "," << static_cast<int>(p.b) << "," << static_cast<int>(p.a) << ","
                  << p.ts << "," << p.nb_records << ")";
            if (++count != fused.size()) query << ",";
        }

        query << " ON CONFLICT (x, y, z) DO UPDATE SET nb_records = spatial_points.nb_records + EXCLUDED.nb_records";

        // Étape 3 : exécuter
        PGresult* res = PQexec(conn_, query.str().c_str());
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

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "[DB-Spatial-pip] Flush took " << duration << " ms\n";
    }

    PGconn*                conn_;
    std::size_t            batch_size_;
    std::vector<PointRGB>  rows_;
};

} // namespace sudp::db
