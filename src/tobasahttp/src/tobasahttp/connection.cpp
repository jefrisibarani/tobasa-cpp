#include "tobasahttp/connection.h"

namespace tbs {
namespace http {

Connection::Connection()
{
   _startTime = std::chrono::system_clock::now();
}

Connection::~Connection()
{
   _onComplete = nullptr;
   _onError    = nullptr;
   _onTimeOut  = nullptr;
   _onClosed   = nullptr;
}

void Connection::id(ConnectionId id)
{
   _connId = id;
}

ConnectionId Connection::id() const
{
   return _connId;
}

bool Connection::closed() const
{
   return _closed;
}

void Connection::callClose(const std::string& reason)
{
   if (_onComplete)
   {
      _onComplete(this->id(), reason);
   }
}

void Connection::onComplete(OnComplete handler)
{
   _onComplete = std::move(handler);
}

void Connection::onError(OnError handler)
{
   _onError = std::move(handler);
}

void Connection::onTimeOut(OnTimeOut handler)
{
   _onTimeOut = std::move(handler);
}

void Connection::onClosed(OnClosed handler)
{
   _onClosed = std::move(handler);
}  

} // namespace http
} // namespace tbs