#pragma once

#include "tobasasql/exp_imp.h"
#include <memory>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <unordered_map>
#include <string>
#include <tobasa/exception.h>
#include <tobasasql/database_connector.h>
#include <tobasasql/pool_acquire_options.h>
#include <tobasasql/settings.h>

namespace tbs {
namespace sql {

class ConnectionPool;

namespace detail {
class ConnectionPoolTestAccess;
}

/**
 * @brief RAII connection handle that automatically returns the connection to the pool.
 */
class TBS_SQL_API PooledConnection
{
   std::shared_ptr<ConnectionPool> _connPool;
   std::shared_ptr<DatabaseConnector> _connector;

public:

   PooledConnection() = default;

   PooledConnection(std::shared_ptr<ConnectionPool> pool,
                  std::shared_ptr<DatabaseConnector> conn)
      : _connPool(std::move(pool)), _connector(std::move(conn)) {}

   // Move only
   PooledConnection(PooledConnection&& other) noexcept
      : _connPool(std::move(other._connPool)), _connector(std::move(other._connector)) {}

   PooledConnection& operator=(PooledConnection&& other) noexcept 
   {
      if (this != &other) 
      {
         release();
         _connPool = std::move(other._connPool);
         _connector = std::move(other._connector);
      }
      return *this;
   }

   ~PooledConnection();

   std::shared_ptr<DatabaseConnector> connector() const
   { 
      return _connector; 
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
class TBS_SQL_API ConnectionPool : public std::enable_shared_from_this<ConnectionPool>
{
   friend class PooledConnection;
   // TODO_JEFRI: remove this friend
   friend class detail::ConnectionPoolTestAccess;

   std::string _name;
   size_t      _maxConnections;
   size_t      _currentConnections = 0;
   conf::ConnectorOption _option;
   std::deque<std::shared_ptr<DatabaseConnector>> _connectors;

   struct Waiter
   {
      std::condition_variable condition;
   };
   
   struct WaitState
   {
      std::deque<std::shared_ptr<Waiter>> waiters;
   };

   std::mutex  _mutex;
   // Keep this member in place to preserve the public class layout.
   std::condition_variable _cv;

public:

   ConnectionPool(std::string name,
                  conf::ConnectorOption option,
                  size_t maxConnections = 4);

   /// Acquire a connection from the pool.
   /// If none are available and maxConnections is reached, waits.
   PooledConnection acquire();
   /// Acquire a connection with an optional wait timeout or cancellation token.
   PooledConnection acquire(const PoolAcquireOptions& options);

   bool matches(const conf::ConnectorOption& option, size_t maxConnections) const noexcept;

private:
   std::shared_ptr<WaitState> waitState();
   void notifyFrontWaiter();
   void release(std::shared_ptr<DatabaseConnector> conn);
};


/**
 * @brief Manager that holds multiple named connection pools.
 */
class TBS_SQL_API ConnectionPoolManager
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
