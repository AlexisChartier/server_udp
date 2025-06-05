#pragma once
#include <libpq-fe.h>
#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
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
    SpatialPipeline(PGconn* conn, std::size_t batch = 10'000)
        : conn_(conn), batch_size_(batch) {}
    void push(PointRGB&& p) {
        rows_.emplace_back(std::move(p));
        if (rows_.size() >= batch_size_){ 
            std::cout <<"[DB-Spatial-pip] too many points need to flush";
            flush();
        }
    }
    ~SpatialPipeline() { flush(); }
private:
    void flush() {
        std::cout.setf(std::ios::unitbuf); // flush stdout after each output
        std::cerr.setf(std::ios::unitbuf); // flush stderr after each output
        if (rows_.empty()){ 
            std::cout << "[DB-Spatial-pip] no points to flush";
            return;
        }
        std::cout << "[DB-Spatial-pip] flushing " << rows_.size() << " points\n";
        PGresult* res = PQexec(conn_,
            "COPY spatial_point (x,y,z,color_r,color_g,color_b,color_a,timestamp,nb_records) FROM STDIN WITH (FORMAT text)");
        if (PQresultStatus(res) != PGRES_COPY_IN) {
            std::cerr << "[DB] COPY command failed: " << PQerrorMessage(conn_) << '\n';
            PQclear(res);
            return;
        }
        PQclear(res);
        for (const auto& p : rows_) {
            std::cout << "[DB] Inserting point: "
                      << p.x << ", " << p.y << ", " << p.z << ", "
                      << int(p.r) << ", " << int(p.g) << ", " << int(p.b) << ", " << int(p.a) << ", "
                      << p.ts << '\n';
            // Prepare the row data
            std::ostringstream oss;
            oss << p.x << '\t' << p.y << '\t' << p.z << '\t'
                << int(p.r) << '\t' << int(p.g) << '\t' << int(p.b) << '\t' << int(p.a) << '\t'
                << p.ts << '\t' << 1 << '\n';
            std::string row = oss.str();
            if (PQputCopyData(conn_, row.c_str(), static_cast<int>(row.size())) != 1) {
                std::cerr << "[DB] PQputCopyData failed: " << PQerrorMessage(conn_) << '\n';
            } else {
                std::cout << "[DB] Inserted row: " << row;
            }
        }
        if (PQputCopyEnd(conn_, nullptr) != 1) {
            std::cerr << "[DB] PQputCopyEnd failed: " << PQerrorMessage(conn_) << '\n';
        }
        PGresult* end_res = PQgetResult(conn_);
        if (PQresultStatus(end_res) != PGRES_COMMAND_OK) {
            std::cerr << "[DB] COPY failed: " << PQerrorMessage(conn_) << '\n';
        }
        PQclear(end_res);
        rows_.clear();
    }
    PGconn* conn_;
    std::size_t batch_size_;
    std::vector<PointRGB> rows_;
};

} // namespace sudp::db