#pragma once

#include <iostream>
#include "tobasahttp/type_common.h"
#include "tobasahttp/message.h"
#include "tobasahttp/formbody.h"
#include "tobasahttp/multipart.h"
#include "tobasahttp/query_string.h"
#include "tobasahttp/cookie.h"
#include "tobasahttp/authentication.h"
#include "tobasahttp/path_argument.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */


/**
 * @class Request
 * @brief Represents an HTTP request.
 * 
 * The Request class encapsulates the details of an HTTP request, including
 * the method, path, target, authority, and other relevant information. It
 * provides methods to access and modify these details.
 * 
 * @sa https://datatracker.ietf.org/doc/html/rfc7230
 * @sa https://datatracker.ietf.org/doc/html/rfc7231
 * @sa https://www.rfc-editor.org/rfc/rfc3986
 */
class Request : public Message
{
private:
   friend class RequestSerializer;

   std::string _method      {};     // GET, POST
   std::string _path        {};     // path (does not inlude query component)
   std::string _target      {};
   std::string _line        {};     // e.g GET /path/to/resource?query=123 HTTP/1.1
   // HTTP/1.1 Host header, in HTTP/2 and 3 replaced with :authority
   std::string _authority   {};     // e.g mangapul.net:8085
   bool        _https       {false};
   bool        _multipart   {false};
   std::string _sessionId   {};

   /// MultipartBody 
   MultipartBodyPtr  _multipartBody {nullptr};

   /// Authentication header context.
   AuthContextPtr    _authContext   {nullptr};

   /// Request Path Argument.
   PathArgumentPtr   _pathArgument  {nullptr};

   bool              _isHeadRequest {false};

   HttpVersion       _httpVersion   {HttpVersion::one};

   /// Request id, incremented for each new request in the same connection
   /// we use Parser id as request id
   uint32_t           _id            {0}; 

public:

   Request(HttpVersion httpVersion);
   ~Request();

   /// Is this https request.
   bool isHttps() const;

   void setHttps(bool https);

   /// Indicates wether this request has valid multipart 
   /// content type and valid boundary
   /// To check wether multipart body available, use hasMultipartBody()
   bool hasMultipart() { return _multipart; }
   void setMultipart(bool val) { _multipart=val; }

   /// Is this request contains Multipart body.
   bool hasMultipartBody();

   /** 
    * Get Http request-line
    * \sa https://datatracker.ietf.org/doc/html/rfc7230#section-3.1.1
    * request-line   = method SP request-target SP HTTP-version CRLF
    */
   std::string line() const;

   /** 
    * Set HTTP request-line
    * \param line
    * \sa https://datatracker.ietf.org/doc/html/rfc7230#section-3.1.1
    */
   void line(const std::string& line);

   /** 
    * Get HTTP request method.
    * \sa https://datatracker.ietf.org/doc/html/rfc7231#section-4
    */
   std::string method() const;

   /** 
    * Set HTTP request method.
    * \param method
    * \sa https://datatracker.ietf.org/doc/html/rfc7231#section-4
    */
   void method(const std::string& method);

   /** 
    * Get Http request-target.
    * \sa https://datatracker.ietf.org/doc/html/rfc7230#section-5.3
    */
   std::string target() const;

   /** 
    * Set Http request-target.
    * \param target
    * \sa https://datatracker.ietf.org/doc/html/rfc7230#section-5.3
    */
   void target(const std::string& target);

   /** 
    * Get Http request path.
    * \sa https://www.rfc-editor.org/rfc/rfc3986#section-3.3
    */
   std::string path() const;

   /** 
    * Get Http request path.
    * \sa https://www.rfc-editor.org/rfc/rfc3986#section-3.3
    */
   void path(const std::string& target);

   /**
    * GEt Http authority, e.g: mangapul.net:443
    */
   std::string authority();

   /** 
    * Set Http authority
    * \param authority
    */
   void authority(const std::string& authority);

   /** 
    * Get Http query string.
    * \sa https://www.rfc-editor.org/rfc/rfc3986#section-3.4
    */
   std::string queryString() const;

   /// Get query string store shared pointer.
   QueryStringPtr query() const;

   /// Set MultipartBody.
   void multipartBody(MultipartBodyUPtr body);

   /// Get FormBody shared pointer.
   FormBodyPtr formBody();

   /// Get Cookie shared pointer
   CookiePtr cookie();

   /// Get multipart body.
   MultipartBodyPtr multipartBody();

   /// Get authentication context from http header.
   AuthContextPtr authContext();

   /// Get Request Path argument from http request path.
   PathArgumentPtr pathArgument(const std::string& pathTemplate = {});
   
   std::string baseUrl();

   std::string sessionId() const;

   void sessionId(const std::string& sessionId);

   void isHeadRequest(bool value);
   
   bool isHeadRequest();

   /// Request id, incremented for each new request in the same connection
   /// we use Parser id as request id
   void id(uint32_t value) { _id = value; }
   uint32_t id() const { return _id; } 
};

/** 
 * Http Request serializer.
 * Serialise http request into std::string
 */
class RequestSerializer
{
private:
   Request& _request;

public:
   RequestSerializer(Request& request);
   std::string getString();
};

/** @}*/

} // namespace http
} // namespace tbs