#pragma once
#include <libpq-fe.h>
#include <vector>
#include <cstdint>
#include <string>
#include <sstream>
#include <iostream>

namespace sudp::db {

struct PointRGB
{
    float   x, y, z;
    uint8_t r, g, b, a;
    int64_t ts;  // epoch‑ms
};

class SpatialPipeline
{
public:
    SpatialPipeline(PGconn* conn, std::size_t batch = 1)
        : conn_(conn), batch_size_(batch)
    {
        const char* stmtName = "insert_spatial_point";
        const char* insertSQL =
            "INSERT INTO spatial_points (x, y, z, color_r, color_g, color_b, color_a, timestamp, nb_records) "
            "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9) "
            "ON CONFLICT (x,y,z) DO UPDATE SET nb_records = spatial_points.nb_records + EXCLUDED.nb_records";

        PGresult* prep = PQprepare(conn_, stmtName, insertSQL, 9, nullptr);
        if (PQresultStatus(prep) != PGRES_COMMAND_OK) {
            std::cerr << "[DB] Prepare failed: " << PQerrorMessage(conn_) << '\n';
        }
        PQclear(prep);
    }

    void push(PointRGB&& p) {
        rows_.emplace_back(std::move(p));
        if (rows_.size() >= batch_size_) {
            flush();
        }
    }

    ~SpatialPipeline() { flush(); }

private:
    void flush() {
        if (rows_.empty()) return;
        std::cout << "[DB-Spatial-pip] flushing " << rows_.size() << " points\n";
        const char* stmtName = "insert_spatial_point";

        for (const auto& p : rows_) {
            const char* values[9];
            int lengths[9];
            int formats[9];

            std::string x = std::to_string(static_cast<int>(p.x));
            std::string y = std::to_string(static_cast<int>(p.y));
            std::string z = std::to_string(static_cast<int>(p.z));
            std::string r = std::to_string(p.r);
            std::string g = std::to_string(p.g);
            std::string b = std::to_string(p.b);
            std::string a = std::to_string(p.a);
            std::string ts = std::to_string(p.ts);
            std::string count = "1";

            values[0] = x.c_str();
            values[1] = y.c_str();
            values[2] = z.c_str();
            values[3] = r.c_str();
            values[4] = g.c_str();
            values[5] = b.c_str();
            values[6] = a.c_str();
            values[7] = ts.c_str();
            values[8] = count.c_str();

            for (int i = 0; i < 9; ++i) {
                lengths[i] = 0;
                formats[i] = 0;  // text format
            }
            PGresult* res = PQexecPrepared(conn_, stmtName, 9, values, lengths, formats, 0);
            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                std::cerr << "[DB] Insert failed: " << PQerrorMessage(conn_) << '\n';
            }
            PQclear(res);
        }
        rows_.clear();
    }

    PGconn* conn_;
    std::size_t batch_size_;
    std::vector<PointRGB> rows_;
};

} // namespace sudp::db