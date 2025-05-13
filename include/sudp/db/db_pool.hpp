#pragma once
/**
 *  Pool « pauvre mais thread-safe » de connexions libpq.
 *  Un thread worker ↔ une connexion PostgreSQL.
 *
 *  Usage :
 *      db::DbPool pool("postgres://user:pwd@host/db", 4);
 *      {
 *          auto c = pool.acquire();        // RAII
 *          pqxx::work tx(*c);
 *          ...
 *      }                                   // retour auto dans le pool
 */
#include <libpq-fe.h>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace sudp::db
{
using PGconnPtr = std::unique_ptr<PGconn, void(*)(PGconn*)>;
class DbPool
{
public:
    DbPool(std::string conninfo, std::size_t size);

    /** RAII wrapper ------------------------------------------------- */
    class Guard
    {
    public:
        Guard(DbPool& p, PGconn* c) : pool_{p}, conn_{c} {}
        Guard(Guard&& g) noexcept
            : pool_{g.pool_}, conn_{std::exchange(g.conn_, nullptr)} {}
        ~Guard() { if (conn_) pool_.release(conn_); }

        PGconn* operator*()  noexcept { return conn_; }
        PGconn* operator->() noexcept { return  conn_; }

        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
    private:
        DbPool&            pool_;
        PGconn*  conn_;
    };

    /** bloque tant qu’une connexion n’est pas libre */
    [[nodiscard]] Guard acquire();

    /* non copiable */
    DbPool(const DbPool&) = delete;
    DbPool& operator=(const DbPool&) = delete;
private:
    void release(PGconn*);

    std::vector<PGconnPtr> conns_;
    std::mutex              m_;
    std::condition_variable cv_;
    std::vector<bool>       busy_;
};
} // namespace sudp::db