#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>

namespace tbs {
namespace sql {

class ConnectionPool;
class PoolAcquireCancellationSource;

/// Thrown when a pool wait reaches its configured timeout.
class PoolAcquireTimeoutError : public std::runtime_error
{
public:
   using std::runtime_error::runtime_error;
};

/// Thrown when a pool wait is cancelled through its cancellation token.
class PoolAcquireCancelledError : public std::runtime_error
{
public:
   using std::runtime_error::runtime_error;
};

/// A copyable token that lets one or more pool waits observe cancellation.
class PoolAcquireCancellationToken
{
   struct State;
   std::shared_ptr<State> _state;

   explicit PoolAcquireCancellationToken(std::shared_ptr<State> state);

   std::size_t subscribe(std::function<void()> notifyWaiter) const;
   
   void unsubscribe(std::size_t subscription) const;
   
   bool isCancellationRequested() const noexcept;

   friend class PoolAcquireCancellationSource;
   friend class ConnectionPool;

public:
   PoolAcquireCancellationToken() = default;
};

/// Creates tokens and cancels pool waits using those tokens.
class PoolAcquireCancellationSource
{
   std::shared_ptr<PoolAcquireCancellationToken::State> _state;

public:
   PoolAcquireCancellationSource();

   /// Get a token for an acquisition request.
   PoolAcquireCancellationToken token() const;
   /// Cancel all current and future waits using tokens from this source.
   void cancel() noexcept;
};

/// Optional timeout and cancellation settings for one pool acquisition.
struct PoolAcquireOptions
{
   /// Maximum time to wait for pool capacity; unset means wait indefinitely.
   std::optional<std::chrono::steady_clock::duration> timeout;
   /// Cancellation applies only while waiting for a pool slot.
   PoolAcquireCancellationToken cancellationToken;
};

} // namespace sql
} // namespace tbs