#pragma once

#include <iostream>
#include "tobasahttp/headers.h"

namespace tbs {
namespace http {

/** 
 * \ingroup HTTP
 * \brief HTTP Message.
 * Base class for Request and Response
 */
class Message
{
protected:
   uint16_t    _majorVersion {1};
   uint16_t    _minorVersion {1};
   Headers     _headers      {};
   std::string _content      {};

public:
   Message() = default;
   virtual ~Message() = default;
   
   uint16_t     majorVersion()  const noexcept { return _majorVersion; }
   uint16_t     minorVersion()  const noexcept { return _minorVersion; }
   Headers&     headers()             { return _headers; }
   const std::string& content() const noexcept { return _content; }

   std::string contentType();

   size_t contentLength();

   void majorVersion(uint16_t value);

   void minorVersion(uint16_t value);

   void headers(Headers headers);

   virtual void content(const std::string& content);

   void setHeaderContentType(const std::string& value);

   void setHeaderContentLength(size_t length);

   // add a header with  with name 'name' to 'value'
   // this way we allow more than one header with the same name
   void addHeader(const std::string& name, const std::string& value);

   // set the only header with name 'name' to 'value'
   // this way we only allow one header with the that name
   void setHeader(const std::string& name, const std::string& value);
};

} // namespace http
} // namespace tbs