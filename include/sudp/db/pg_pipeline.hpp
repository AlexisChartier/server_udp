#pragma once

#include <libpq-fe.h>
#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <sstream>

namespace sudp::db {

class PgPipeline {
public:
    explicit PgPipeline(PGconn* conn, std::size_t batch_size = 2048)
        : conn_(conn), batch_size_(batch_size) {
        if (!conn_ || PQstatus(conn_) != CONNECTION_OK) {
            throw std::runtime_error("Invalid PostgreSQL connection");
        }
    }

    ~PgPipeline() {
        flush(true);
    }

    void push(uint8_t drone_id, uint32_t seq, uint8_t flags, const std::vector<uint8_t>& blob) {
        rows_.emplace_back(drone_id, seq, flags, blob);
        if (rows_.size() >= batch_size_) flush(true);
    }

private:
    struct row_t {
        uint8_t drone_id;
        uint32_t seq;
        uint8_t flags;
        std::vector<uint8_t> blob;

        row_t(uint8_t d, uint32_t s, uint8_t f, const std::vector<uint8_t>& b)
            : drone_id(d), seq(s), flags(f), blob(b) {}
    };

    void flush(bool force) {
        std::cout.setf(std::ios::unitbuf); // flush stdout after each output
        std::cerr.setf(std::ios::unitbuf);
        if (rows_.empty() && !force) return;

        PGresult* res = PQexec(conn_, "COPY scans_binary (drone_id, seq, flags, data) FROM STDIN BINARY");
        if (PQresultStatus(res) != PGRES_COPY_IN) {
            PQclear(res);
            throw std::runtime_error("Failed to start COPY BINARY");
        }
        PQclear(res);

        // Write PostgreSQL binary COPY header
        static const char header[] = "PGCOPY\n\377\r\n\0";
        static const char flags[4] = {0, 0, 0, 0}; // no flags
        static const char header_ext[4] = {0, 0, 0, 0}; // header extension
        PQputCopyData(conn_, header, 11);
        PQputCopyData(conn_, flags, 4);
        PQputCopyData(conn_, header_ext, 4);

        for (const auto& row : rows_) {
            std::vector<char> buf;
            write_binary_row(row, buf);
            PQputCopyData(conn_, buf.data(), buf.size());
        }

        // Write end-of-copy marker
        static const char trailer[2] = {-1, -1};
        PQputCopyData(conn_, trailer, 2);

        if (PQputCopyEnd(conn_, nullptr) != 1) {
            throw std::runtime_error("Failed to finish COPY BINARY");
        }

        PGresult* end_res = PQgetResult(conn_);
        if (PQresultStatus(end_res) != PGRES_COMMAND_OK) {
            std::string err = PQresultErrorMessage(end_res);
            PQclear(end_res);
            throw std::runtime_error("COPY failed: " + err);
        }
        PQclear(end_res);

        rows_.clear();
    }

    static void write_binary_row(const row_t& row, std::vector<char>& out) {
        auto write_field = [&](const void* data, int len) {
            int32_t len_net = htonl(len);
            out.insert(out.end(), reinterpret_cast<char*>(&len_net), reinterpret_cast<char*>(&len_net) + 4);
            out.insert(out.end(), reinterpret_cast<const char*>(data), reinterpret_cast<const char*>(data) + len);
        };

        int16_t nfields = htons(4);
        out.insert(out.end(), reinterpret_cast<char*>(&nfields), reinterpret_cast<char*>(&nfields) + 2);

        // drone_id
        uint8_t d_id = row.drone_id;
        write_field(&d_id, sizeof(d_id));

        // seq
        uint32_t s = htonl(row.seq);
        write_field(&s, sizeof(s));

        // flags
        uint8_t f = row.flags;
        write_field(&f, sizeof(f));

        // blob
        write_field(row.blob.data(), row.blob.size());
    }

    PGconn* conn_;
    std::size_t batch_size_;
    std::vector<row_t> rows_;
};

} // namespace sudp::db