#pragma once
#include <libpq-fe.h>
#include <vector>
#include <cstdint>
#include <string>
#include <sstream>
#include <iostream>
#include <chrono>

namespace sudp::db {

struct PointRGB {
    float x, y, z;
    uint8_t r, g, b, a;
    int nb_records = 1; // default to 1 for the first insert
    int64_t ts; // epoch-ms
};

class SpatialPipeline {
public:
    SpatialPipeline(PGconn* conn, std::size_t batch = 1000)
        : conn_(conn), batch_size_(batch) {}

    void push(PointRGB&& p) {
        rows_.emplace_back(std::move(p));
        if (rows_.size() >= batch_size_) {
            flush();
        }
    }

    ~SpatialPipeline() {
        flush();
    }

private:
    void flush() {
        if (rows_.empty()) return;

        auto start = std::chrono::high_resolution_clock::now();

        std::cout << "[DB-Spatial-pip] flushing " << rows_.size() << " points\n";

        std::ostringstream query;
        query << "INSERT INTO spatial_points "
              << "(x, y, z, color_r, color_g, color_b, color_a, timestamp, nb_records) VALUES ";

        for (std::size_t i = 0; i < rows_.size(); ++i) {
            const auto& p = rows_[i];
            query << "("
                  << static_cast<int>(p.x) << ","
                  << static_cast<int>(p.y) << ","
                  << static_cast<int>(p.z) << ","
                  << static_cast<int>(p.r) << ","
                  << static_cast<int>(p.g) << ","
                  << static_cast<int>(p.b) << ","
                  << static_cast<int>(p.a) << ","
                  << p.ts << ","
                  << 1 << ")";
            if (i != rows_.size() - 1)
                query << ",";
        }

        query << " ON CONFLICT (x,y,z) DO UPDATE SET nb_records = spatial_points.nb_records + EXCLUDED.nb_records";

        PGresult* res = PQexec(conn_, query.str().c_str());
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::cerr << "[DB] Bulk insert failed: " << PQerrorMessage(conn_) << '\n';
        } else {
            std::cout << "[DB] Bulk insert succeeded\n";
        }

        PQclear(res);
        rows_.clear();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "[DB-Spatial-pip] Flush took " << duration_ms << " ms\n";
    }

    PGconn* conn_;
    std::size_t batch_size_;
    std::vector<PointRGB> rows_;
};

} // namespace sudp::db

