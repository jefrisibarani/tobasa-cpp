#include <tobasasql/sql_connection_pool.h>

namespace tbs {
namespace sql {

PooledConnection::~PooledConnection() 
{   
   release();
   Logger::logT("[sql] PooledConnection destructed");
}

 void PooledConnection::release()
{
   if (_pool && _conn)
   {
      Logger::logT("[sql] PooledConnection releasing connector {}", _conn->name());

      auto tmp = std::move(_conn);
      _pool->release(std::move(tmp)); // hand ownership to pool
   }
}


PooledConnection ConnectionPool::acquire() 
{
   std::unique_lock lock(_mutex);

   while (_pool.empty() && _currentConnections >= _maxConnections)
      _cv.wait(lock);

   if (!_pool.empty()) 
   {
      auto conn = _pool.front();
      _pool.pop_front();
      lock.unlock();

      if (!conn->connected())
         conn->connect();

      return PooledConnection(shared_from_this(), conn);
   }

   // Create new connection
   _currentConnections++;
   lock.unlock();

   Logger::logD("[sql] Creating new pooled db connection for pool {}", _name);

   auto conn = std::make_shared<DatabaseConnector>(_option, _name);
   conn->initSqlDriver();

   if (!conn->connect())
      throw AppException("Failed to connect database for pool: " + _name);

   return PooledConnection(shared_from_this(), conn);
}


void ConnectionPool::release(std::shared_ptr<DatabaseConnector> conn) 
{
   std::lock_guard lock(_mutex);
   _pool.push_back(std::move(conn));
   _cv.notify_one();
}


std::shared_ptr<ConnectionPool> ConnectionPoolManager::getOrCreatePool(
    const std::string& name,
    const conf::ConnectorOption& option,
    size_t maxConnections)
{
   std::lock_guard lock(_mutex);
   auto it = _pools.find(name);
   if (it != _pools.end())
      return it->second;

   Logger::logD("[sql] Creating new connection pool: {}", name);
   auto pool = std::make_shared<ConnectionPool>(name, option, maxConnections);
   _pools[name] = pool;
   return pool;
}


void ConnectionPoolManager::clear()
{
   std::unordered_map<std::string, std::shared_ptr<ConnectionPool>> tmp;

   {
      std::lock_guard lock(_mutex);
      tmp.swap(_pools);  // Move out safely
   }

   // Now, _mutex is unlocked — destruction happens outside the lock.
   tmp.clear();
}

} // namespace sql
} // namespace tbs