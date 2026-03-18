#pragma once

#include <memory>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <unordered_map>
#include <string>
#include <tobasa/exception.h>
#include <tobasasql/database_connector.h>
#include <tobasasql/settings.h>

namespace tbs {
namespace sql {

class ConnectionPool;

/**
 * @brief RAII connection handle that automatically returns the connection to the pool.
 */
class PooledConnection 
{
    std::shared_ptr<ConnectionPool> _pool;
    std::shared_ptr<DatabaseConnector> _conn;

public:

   PooledConnection() = default;

   PooledConnection(std::shared_ptr<ConnectionPool> pool,
                  std::shared_ptr<DatabaseConnector> conn)
      : _pool(std::move(pool)), _conn(std::move(conn)) {}

   // Move only
   PooledConnection(PooledConnection&& other) noexcept
      : _pool(std::move(other._pool)), _conn(std::move(other._conn)) {}

   PooledConnection& operator=(PooledConnection&& other) noexcept 
   {
      if (this != &other) 
      {
         release();
         _pool = std::move(other._pool);
         _conn = std::move(other._conn);
      }
      return *this;
   }

   ~PooledConnection();

   std::shared_ptr<DatabaseConnector> conn() const
   { 
      return _conn; 
   }

   void release();

private:

   // Disallow copying
   PooledConnection(const PooledConnection&) = delete;
   PooledConnection& operator=(const PooledConnection&) = delete;
};


/**
 * @brief Thread-safe database connection pool.
 * Holds multiple DatabaseConnector instances and hands out pooled connections.
 */
class ConnectionPool : public std::enable_shared_from_this<ConnectionPool> 
{
   friend class PooledConnection;

   std::string             _name;
   conf::ConnectorOption   _option;
   std::mutex              _mutex;
   std::condition_variable _cv;

   size_t                  _maxConnections;
   size_t                  _currentConnections = 0;
   
   std::deque<std::shared_ptr<DatabaseConnector>> _pool;

public:

   ConnectionPool(std::string name,
                  conf::ConnectorOption option,
                  size_t maxConnections = 4)
      : _name(std::move(name))
      , _option(std::move(option))
      , _maxConnections(maxConnections) {}

   /**
    * @brief Acquire a connection from the pool.
    *        If none are available and maxConnections is reached, waits.
    */
   PooledConnection acquire();

private:
   void release(std::shared_ptr<DatabaseConnector> conn);
};


/**
 * @brief Manager that holds multiple named connection pools.
 */
class ConnectionPoolManager 
{
   std::unordered_map<std::string, std::shared_ptr<ConnectionPool>> _pools;
   std::mutex _mutex;

public:

   /**
    * @brief Retrieve or create a connection pool by name.
    */
   std::shared_ptr<ConnectionPool> getOrCreatePool(
      const std::string& name,
      const conf::ConnectorOption& option,
      size_t maxConnections = 4 );

   /**
    * @brief Clear all pools.
    */
   void clear();

};

} // namespace sql
} // namespace tbs
