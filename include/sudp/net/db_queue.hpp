#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <optional>
#include <condition_variable>

#include "sudp/db/spatial_pipeline.hpp"   // ← PointRGB

namespace sudp::net {

using sudp::db::PointRGB;

struct DbItem
{
    uint8_t              drone_id;
    uint32_t             seq;
    uint8_t              flags;
    std::vector<uint8_t> payload;   // blob binaire complet
};

/* -------- lot de points pour spatial_point ------------------ */
struct PointBatch
{
    std::vector<PointRGB> pts;
};

class DbQueue
{
public:
    /* -------- blobs (scans_binary) --------------------------- */
    void push(DbItem&& it)
    { std::lock_guard lk(m_); blobs_.push(std::move(it)); cv_.notify_one(); }

    std::optional<DbItem> try_pop_blob()
    {
        std::lock_guard lk(m_); if(blobs_.empty()) return std::nullopt;
        auto it = std::move(blobs_.front()); blobs_.pop(); return it;
    }

    /* -------- points (spatial_point) ------------------------- */
    void push_points(std::vector<PointRGB>&& v){ 
        std::cout.setf(std::ios::unitbuf); // flush stdout after each output
        std::cerr.setf(std::ios::unitbuf); // flush stderr after each output

        std::cout << "[DB] Received batch with " << v.size() << " points and pushing\n";
                  
        std::lock_guard lk(m_); batches_.emplace(PointBatch{std::move(v)}); cv_.notify_one(); 
    }

    std::optional<PointBatch> try_pop_batch()
    {
        std::lock_guard lk(m_); if(batches_.empty()) return std::nullopt;
        auto b = std::move(batches_.front()); batches_.pop(); return b;
    }

private:
    std::queue<DbItem>      blobs_;
    std::queue<PointBatch>  batches_;

    std::mutex              m_;
    std::condition_variable cv_;
};

} // namespace sudp::net

