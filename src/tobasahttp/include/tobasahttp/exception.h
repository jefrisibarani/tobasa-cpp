#pragma once

#include <stdexcept>

namespace tbs {
namespace http {

/** 
 * \ingroup HTTP
 * \brief HTTP Exception
 */
class Exception
   : public std::runtime_error
{
public:
   Exception( const char* message, int32_t streamId = -1)
      : std::runtime_error { message }
      , _streamId {streamId} {}

   Exception( const std::string& message, int32_t streamId = -1)
      :	std::runtime_error { message }
      , _streamId {streamId} {}

   Exception( std::string_view message, int32_t streamId = -1 )
      :	std::runtime_error { std::string{ message.data(), message.size() } }
      , _streamId {streamId} {}

   int32_t streamId() const { return _streamId; }

private:
   int32_t _streamId = -1;
};

} // namespace http
} // namespace tbs