#include "tobasahttp/util.h"
#include "tobasahttp/request.h"

namespace tbs {
namespace http {

Request::Request(HttpVersion httpVersion)
   : Message()
   , _httpVersion   {httpVersion}
   , _multipartBody {nullptr}
   , _authContext   {nullptr}
   , _pathArgument  {nullptr}
{
}

Request::~Request()
{
   // Remove multipart body temporary files
   if (hasMultipartBody())
      _multipartBody->cleanup(true);
}

bool Request::isHttps() const { return _https; }

void Request::setHttps(bool https)
{
   _https = https;
}

bool Request::hasMultipartBody()
{
   if (_multipartBody != nullptr)
      return ! _multipartBody->empty();
   else
      return false;
}

std::string Request::line() const { return _line; }

void Request::line(const std::string& line)
{
   _line = line;
}

std::string Request::method() const { return _method; }

void Request::method(const std::string& method)
{
   _method = method;
}

std::string Request::target() const { return _target; }

void Request::target(const std::string& target)
{
   // TODO_JEFRI: Parse request-target, to get path, host, port
   _target = target;

   std::string out;
   if (percentDecode(_target, out))
   {
      // Extract and get path from uri, remove query string
      _path = urlGetPath(out);
   }
}

std::string Request::path() const
{
   return _path;
}

void Request::path(const std::string& path)
{
   std::string out;
   if (percentDecode(path, out))
   {
      _path = urlGetPath(out);
   }
}

std::string Request::authority()
{
   return _authority;
}

void Request::authority(const std::string& authority)
{
   _authority = authority;
}

std::string Request::queryString() const
{
   return urlGetQueryString(_target);
}

QueryStringPtr Request::query() const
{
   auto qryStr = urlGetQueryString(_target);
   if (qryStr.length()>0)
   {
      auto qry = std::make_shared<QueryString>();
      qry->parse(qryStr);
      return qry;
   }
   else
      return std::make_shared<QueryString>();
}

PathArgumentPtr Request::pathArgument(const std::string& pathTemplate)
{
   if (_pathArgument != nullptr)
      return _pathArgument;
   else
   {
      if (pathTemplate.length() > 0 )
      {
         auto pathArgument = std::make_shared<PathArgument>();
         pathArgument->parse(pathTemplate, _path);
         if (pathArgument->match())
         {
            _pathArgument = pathArgument;
            return _pathArgument;
         }
      }

      return nullptr;
   }
}

FormBodyPtr Request::formBody()
{
   if ( contentType() == "application/x-www-form-urlencoded")
   {
      auto formbody = std::make_shared<FormBody>();
      formbody->parse(_content);
      return formbody;
   }
   else
      return nullptr;
}

CookiePtr Request::cookie()
{
#ifdef TOBASA_HTTP_USE_HTTP2
   if (_httpVersion == HttpVersion::two)
   {
      auto cookie = std::make_shared<Cookie>();
      cookie->fromCollection(this->headers());
      return cookie;
   }
   else
   {
      auto cookieHeaderData = this->headers().value("Cookie");
      if (!cookieHeaderData.empty())
      {
         auto cookie = std::make_shared<Cookie>();
         cookie->parse(cookieHeaderData);
         return cookie;
      }
      else
         return std::make_shared<Cookie>();
   }
#else
   auto cookieHeaderData = this->headers().value("Cookie");
   if (!cookieHeaderData.empty())
   {
      auto cookie = std::make_shared<Cookie>();
      cookie->parse(cookieHeaderData);
      return cookie;
   }
   else
      return std::make_shared<Cookie>();
#endif
}

void Request::multipartBody(MultipartBodyUPtr body)
{
   _multipartBody = std::move(body);
}

MultipartBodyPtr Request::multipartBody()
{
   return _multipartBody;
}

AuthContextPtr Request::authContext()
{
   if (_authContext)
      return _authContext;
   else
   {
      FieldPtr field = this->headers().field(HeaderName::AUTHORIZATION);
      if (field == nullptr)
         field = this->headers().field(HeaderName::SEC_WEBSOCKET_PROTOCOL);

      if (field != nullptr)
      {
         _authContext = parseAuthorizationHeader(field->value());
         _authContext->headerFound = true;
      }
      else
         _authContext = std::make_shared<AuthContext>();

      return _authContext;
   }
}

std::string Request::baseUrl()
{
   std::string baseUrl;

   if ( this->isHttps() )
      baseUrl = "https";
   else
      baseUrl = "http";
   
   baseUrl += "://";

   if (_httpVersion == HttpVersion::one)
   {
      auto hostField = this->headers().field(HeaderName::HOST);
      if (hostField != nullptr)
         baseUrl += hostField->value();
   }
   else
   {
      baseUrl += _authority;
   }
   
   return baseUrl;
}

std::string Request::sessionId() const
{
   return _sessionId;
}

void Request::sessionId(const std::string& sessionId)
{
   _sessionId = sessionId;
}

void Request::isHeadRequest(bool value)
{
   _isHeadRequest = value;
}

bool Request::isHeadRequest()
{
   return _isHeadRequest;
}

RequestSerializer::RequestSerializer(Request& request)
   : _request(request)
{}

std::string RequestSerializer::getString()
{
   if ( _request._content.size() > 0) {
      _request.addHeader("Content-Length", std::to_string(_request._content.size()) );
   }

   // HEAD method is similar to GET, but we should not return the body of the response
   if (_request._isHeadRequest )
   {
      // empty response body for HEAD Request
      _request._content = "";
   }   

   // first, build a request line (eg. GET /version/index.html HTTP/1.1 )
   std::stringstream stream;
   stream 	<< _request._method
            << " "
            << _request._path
            << " HTTP/"
            << _request._majorVersion
            << "."
            << _request._minorVersion
            << "\r\n" ;

   // add headers
   for ( size_t i = 0; i < _request._headers.size(); i++ )
   {
      auto f = _request._headers.field(i);
      if ( f != nullptr )
         stream <<  f->name() << ": " << f->value() << "\r\n";
   }

   stream << "\r\n";

   // add body
   if ( _request._content.size() > 0 )
      stream << std::string(_request._content.data(), _request._content.size());

   return std::move(stream.str());
}

} // namespace http
} // namespace tbs