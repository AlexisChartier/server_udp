#pragma once
/**
 *  Pool « pauvre mais thread-safe » de connexions libpqxx.
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
#include <pqxx/pqxx>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace sudp::db
{
class DbPool
{
public:
    DbPool(std::string conninfo, std::size_t size);

    /** RAII wrapper ------------------------------------------------- */
    class Guard
    {
    public:
        Guard(DbPool& p, pqxx::connection* c) : pool_{p}, conn_{c} {}
        Guard(Guard&& g) noexcept
            : pool_{g.pool_}, conn_{std::exchange(g.conn_, nullptr)} {}
        ~Guard() { if (conn_) pool_.release(conn_); }

        pqxx::connection& operator*()  noexcept { return *conn_; }
        pqxx::connection* operator->() noexcept { return  conn_; }

        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
    private:
        DbPool&            pool_;
        pqxx::connection*  conn_;
    };

    /** bloque tant qu’une connexion n’est pas libre */
    [[nodiscard]] Guard acquire();

    /* non copiable */
    DbPool(const DbPool&) = delete;
    DbPool& operator=(const DbPool&) = delete;
private:
    void release(pqxx::connection*);

    std::vector<std::unique_ptr<pqxx::connection>> conns_;
    std::mutex              m_;
    std::condition_variable cv_;
    std::vector<bool>       busy_;
};
} // namespace sudp::db