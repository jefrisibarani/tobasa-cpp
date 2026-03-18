#pragma once

#include <functional>
#include <string>
#include <tobasa/non_copyable.h>

namespace tbs {
namespace lis {

/** \ingroup LIS
 * Connection read buffer size
 */
constexpr auto CONNECTION_READ_BUF_SIZE = 4096;

/** \ingroup LIS
 * Connection class.
 */
class Connection : private NonCopyable
{
public:
   virtual ~Connection() = default;
   virtual void connect() = 0;
   virtual void disConnect() = 0;
   virtual void send(const std::string& value) = 0;
   virtual void asyncSend(const std::string& data) = 0;
   virtual bool connected() = 0;

   std::function<void(const std::string&)> onReceiveData;
   std::function<void(const std::string&)> onConnectFailed;
   std::function<void(const std::string&)> onConnected;
};

} // namespace lis
} // namespace tbs 