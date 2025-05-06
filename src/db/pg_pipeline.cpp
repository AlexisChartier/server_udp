#include "db/pg_pipeline.hpp"
#include <libpq-fe.h>
#include <cstring>
#include <stdexcept>

namespace sudp::db {

static void write_binary_uint32(std::string& buffer, uint32_t value) {
    uint32_t net = htonl(value);
    buffer.append(reinterpret_cast<const char*>(&net), sizeof(net));
}

static void write_binary_int16(std::string& buffer, int16_t value) {
    uint16_t net = htons(static_cast<uint16_t>(value));
    buffer.append(reinterpret_cast<const char*>(&net), sizeof(net));
}

static void write_copy_header(std::string& buffer) {
    static const char signature[11] = "PGCOPY\n\377\r\n\0";
    buffer.append(signature, 11);
    buffer.append(4, '\0'); // Flags
    buffer.append(4, '\0'); // Header extension length
}

static void write_copy_footer(std::string& buffer) {
    buffer.append("\xFF\xFF", 2); // End-of-copy marker
}

void PgPipeline::insert_bulk(const std::vector<common_dmw::Voxel>& voxels) {
    auto conn_guard = pool_.acquire();
    PGconn* raw_conn = *conn_guard;
    
    if (PQputCopyStart(raw_conn, "COPY voxels (x,y,z,r,g,b) FROM STDIN BINARY") != 1) {
        throw std::runtime_error("COPY start failed: " + std::string(PQerrorMessage(raw_conn)));
    }

    std::string buffer;
    buffer.reserve(4096);
    write_copy_header(buffer);

    for (const auto& v : voxels) {
        buffer.append("\0\6", 2); // Number of fields: 6 (int16_t big-endian)

        uint32_t x, y, z;
        common_dmw::decode_morton10(v.morton, x, y, z);

        for (int val : {
                static_cast<int>(x),
                static_cast<int>(y),
                static_cast<int>(z),
                static_cast<int16_t>((v.rgb >> 16) & 0xFF),
                static_cast<int16_t>((v.rgb >> 8) & 0xFF),
                static_cast<int16_t>(v.rgb & 0xFF)
            }) {
            std::string field;
            if (val > 32767 || val < -32768) {
                uint32_t i = htonl(val);
                field.assign(reinterpret_cast<const char*>(&i), sizeof(i));
                buffer.append("\0\0\0\4", 4); // length = 4
                buffer.append(field);
            } else {
                int16_t s = htons(static_cast<int16_t>(val));
                field.assign(reinterpret_cast<const char*>(&s), sizeof(s));
                buffer.append("\0\0\0\2", 4); // length = 2
                buffer.append(field);
            }
        }
    }

    write_copy_footer(buffer);

    if (PQputCopyData(raw_conn, buffer.data(), static_cast<int>(buffer.size())) != 1) {
        throw std::runtime_error("COPY data failed: " + std::string(PQerrorMessage(raw_conn)));
    }

    if (PQputCopyEnd(raw_conn, nullptr) != 1) {
        throw std::runtime_error("COPY end failed: " + std::string(PQerrorMessage(raw_conn)));
    }

    PGresult* res = PQgetResult(raw_conn);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::string err = PQresultErrorMessage(res);
        PQclear(res);
        throw std::runtime_error("COPY command error: " + err);
    }
    PQclear(res);
}

} // namespace sudp::db

