#include "db/db_pool.hpp"
#include <stdexcept>

namespace sudp::db
{

DbPool::DbPool(std::string conninfo, std::size_t size)
{
    if (size == 0)
        throw std::invalid_argument("DbPool: size must be > 0");

    for (std::size_t i = 0; i < size; ++i)
    {
        conns_.emplace_back(PQconnectdb(conninfo.c_str()), PQfinish);
        busy_.push_back(false);
    }
}

DbPool::Guard DbPool::acquire()
{
    std::unique_lock lock(m_);
    cv_.wait(lock, [&] {
        for (bool b : busy_) if (!b) return true;
        return false;
    });

    for (std::size_t i = 0; i < conns_.size(); ++i)
    {
        if (!busy_[i])
        {
            busy_[i] = true;
            return Guard(*this, conns_[i].get());
        }
    }

    throw std::runtime_error("DbPool: no available connection after wait");
}

void DbPool::release(pqxx::connection* conn)
{
    std::lock_guard lock(m_);
    for (std::size_t i = 0; i < conns_.size(); ++i)
    {
        if (conns_[i].get() == conn)
        {
            busy_[i] = false;
            cv_.notify_one();
            return;
        }
    }
    throw std::runtime_error("DbPool: trying to release unknown connection");
}

} 