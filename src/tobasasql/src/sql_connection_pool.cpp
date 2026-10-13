#include <tobasasql/sql_connection_pool.h>

#include <algorithm>
#include <atomic>
#include <stdexcept>
#include <unordered_map>

namespace tbs {
namespace sql {

struct PoolAcquireCancellationToken::State
{
   std::atomic<bool> cancelled{false};
   std::mutex mutex;
   std::size_t nextSubscription = 1;
   std::unordered_map<std::size_t, std::function<void()>> waiters;
};

PoolAcquireCancellationToken::PoolAcquireCancellationToken(std::shared_ptr<State> state)
   : _state(std::move(state))
{
}

std::size_t PoolAcquireCancellationToken::subscribe(std::function<void()> notifyWaiter) const
{
   if (!_state)
      return 0;

   std::lock_guard lock(_state->mutex);
   if (_state->cancelled.load(std::memory_order_acquire))
      return 0;

   auto subscription = _state->nextSubscription++;
   if (subscription == 0)
      subscription = _state->nextSubscription++;

   _state->waiters.emplace(subscription, std::move(notifyWaiter));
   return subscription;
}

void PoolAcquireCancellationToken::unsubscribe(std::size_t subscription) const
{
   if (!_state || subscription == 0)
      return;

   std::lock_guard lock(_state->mutex);
   _state->waiters.erase(subscription);
}

bool PoolAcquireCancellationToken::isCancellationRequested() const noexcept
{
   return _state && _state->cancelled.load(std::memory_order_acquire);
}

PoolAcquireCancellationSource::PoolAcquireCancellationSource()
   : _state(std::make_shared<PoolAcquireCancellationToken::State>())
{
}

PoolAcquireCancellationToken PoolAcquireCancellationSource::token() const
{
   return PoolAcquireCancellationToken(_state);
}

void PoolAcquireCancellationSource::cancel() noexcept
{
   if (!_state)
      return;

   decltype(_state->waiters) waiters;
   {
      std::lock_guard lock(_state->mutex);
      if (_state->cancelled.exchange(true, std::memory_order_acq_rel))
         return;

      waiters.swap(_state->waiters);
   }

   for (auto& waiter : waiters)
   {
      try
      {
         waiter.second();
      }
      catch (...)
      {
      }
   }
}

PooledConnection::~PooledConnection() 
{
   release();
#if !defined(NDEBUG)
   Logger::logT("[sql] PooledConnection destructed");
#endif
}

 void PooledConnection::release()
{
   if (_connPool && _connector)
   {
#if !defined(NDEBUG)
      Logger::logT("[sql] PooledConnection releasing connector {}", _connector->name());
#endif

      auto tmp = std::move(_connector);
      _connPool->release(std::move(tmp)); // hand ownership to pool
   }
}


ConnectionPool::ConnectionPool(std::string name,
   conf::ConnectorOption option,
   size_t maxConnections)
   : _name(std::move(name))
   , _option(std::move(option))
   , _maxConnections(maxConnections) 
{
   if (_maxConnections == 0)
      throw std::invalid_argument("Database connection pool size must be greater than zero");
}

bool ConnectionPool::matches(const conf::ConnectorOption& option, size_t maxConnections) const noexcept
{
   return _option == option && _maxConnections == maxConnections;
}

PooledConnection ConnectionPool::acquire() 
{
   return acquire(PoolAcquireOptions{});
}

PooledConnection ConnectionPool::acquire(const PoolAcquireOptions& options)
{
   auto pool = shared_from_this();
   auto waitState = this->waitState();
   auto discardConnection = [this](std::shared_ptr<DatabaseConnector>& conn)
   {
      conn.reset();
      {
         std::lock_guard lock(_mutex);
         --_currentConnections;
      }
      notifyFrontWaiter();
   };

   using Clock = std::chrono::steady_clock;
   std::optional<Clock::time_point> deadline;
   if (options.timeout)
   {
      if (*options.timeout < Clock::duration::zero())
         throw std::invalid_argument("Pool acquisition timeout cannot be negative");

      const auto now = Clock::now();
      const auto remaining = Clock::time_point::max() - now;
      deadline = *options.timeout >= remaining
         ? Clock::time_point::max() : now + *options.timeout;
   }

   auto waiter = std::make_shared<Waiter>();
   std::size_t cancellationSubscription = 0;
   bool isQueued = false;
   std::shared_ptr<DatabaseConnector> conn;
   bool createConnection = false;
   std::unique_lock lock(_mutex);

   auto removeWaiter = [&]()
   {
      bool wasFront = false;
      if (isQueued)
      {
         auto position = std::find(waitState->waiters.begin(), waitState->waiters.end(), waiter);
         wasFront = position == waitState->waiters.begin();
         if (position != waitState->waiters.end())
            waitState->waiters.erase(position);
            
         isQueued = false;
      }
      if (cancellationSubscription != 0)
      {
         options.cancellationToken.unsubscribe(cancellationSubscription);
         cancellationSubscription = 0;
      }

      if (wasFront)
      {
         if (!waitState->waiters.empty())
            waitState->waiters.front()->condition.notify_one();
      }
   };

   try
   {
      if (options.cancellationToken.isCancellationRequested())
         throw PoolAcquireCancelledError("Pool acquisition was cancelled");

      const bool canAcquireImmediately = waitState->waiters.empty()
         && (!_connectors.empty() || _currentConnections < _maxConnections);

      if (!canAcquireImmediately)
      {
         if (deadline && Clock::now() >= *deadline)
            throw PoolAcquireTimeoutError("Timed out waiting for a pooled connection");

         std::weak_ptr<Waiter> weakWaiter = waiter;
         cancellationSubscription = options.cancellationToken.subscribe([pool, weakWaiter]
         {
            std::lock_guard lock(pool->_mutex);
            if (auto activeWaiter = weakWaiter.lock())
               activeWaiter->condition.notify_one();
         });

         if (options.cancellationToken.isCancellationRequested())
            throw PoolAcquireCancelledError("Pool acquisition was cancelled");

         waitState->waiters.push_back(waiter);
         isQueued = true;

         for (;;)
         {
            if (options.cancellationToken.isCancellationRequested())
               throw PoolAcquireCancelledError("Pool acquisition was cancelled");
               
            if (deadline && Clock::now() >= *deadline)
               throw PoolAcquireTimeoutError("Timed out waiting for a pooled connection");

            if (waitState->waiters.front() == waiter
               && (!_connectors.empty() || _currentConnections < _maxConnections))
               break;

            if (deadline)
               waiter->condition.wait_until(lock, *deadline);
            else
               waiter->condition.wait(lock);
         }
      }

      if (!_connectors.empty())
      {
         conn = std::move(_connectors.front());
         _connectors.pop_front();
      }
      else
      {
         ++_currentConnections;
         createConnection = true;
      }

      removeWaiter();
   }
   catch (...)
   {
      removeWaiter();
      throw;
   }

   lock.unlock();

   if (conn)
   {
      try
      {
         if (!conn->connected() && !conn->connect())
            throw AppException("Failed to reconnect database for pool: " + _name);
      }
      catch (...)
      {
         discardConnection(conn);
         throw;
      }

      return PooledConnection(std::move(pool), std::move(conn));
   }

   if (!createConnection)
      throw std::logic_error("Pool acquisition did not reserve a connection");

#if !defined(NDEBUG)
   Logger::logT("[sql] Creating new pooled db connection for pool {}", _name);
#endif

   std::shared_ptr<DatabaseConnector> createdConnection;
   try
   {
      createdConnection = std::make_shared<DatabaseConnector>(_option, _name);
      createdConnection->initSqlDriver();

      if (!createdConnection->connect())
         throw AppException("Failed to connect database for pool: " + _name);

      return PooledConnection(std::move(pool), std::move(createdConnection));
   }
   catch (...)
   {
      discardConnection(createdConnection);
      throw;
   }
}


void ConnectionPool::release(std::shared_ptr<DatabaseConnector> conn) 
{
   if (!conn->resetTransactionForPool())
   {
      conn.reset();
      {
         std::lock_guard lock(_mutex);
         --_currentConnections;
      }
      notifyFrontWaiter();
      return;
   }

   {
      std::lock_guard lock(_mutex);
      _connectors.push_back(std::move(conn));
   }
   notifyFrontWaiter();
}

void ConnectionPool::notifyFrontWaiter()
{
   auto state = waitState();
   std::lock_guard lock(_mutex);
   if (!state->waiters.empty())
      state->waiters.front()->condition.notify_one();
}

std::shared_ptr<ConnectionPool::WaitState> ConnectionPool::waitState()
{
   static std::mutex registryMutex;
   static std::unordered_map<ConnectionPool*, std::weak_ptr<WaitState>> registry;

   std::lock_guard lock(registryMutex);
   for (auto it = registry.begin(); it != registry.end();)
   {
      if (it->second.expired())
         it = registry.erase(it);
      else
         ++it;
   }

   auto& weakState = registry[this];
   auto state = weakState.lock();
   if (!state)
   {
      state = std::make_shared<WaitState>();
      weakState = state;
   }
   return state;
}


std::shared_ptr<ConnectionPool> ConnectionPoolManager::getOrCreatePool(
   const std::string& name,
   const conf::ConnectorOption& option,
   size_t maxConnections)
{
   std::lock_guard lock(_mutex);
   auto it = _pools.find(name);
   if (it != _pools.end())
   {
      auto& existingPool = it->second;
      if (!existingPool->matches(option, maxConnections))
      {
         throw std::invalid_argument("Connection pool '" + name + "' already exists with different options or size");
      }
      return existingPool;
   }

#if !defined(NDEBUG)
   Logger::logT("[sql] Creating new connection pool: {}", name);
#endif
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