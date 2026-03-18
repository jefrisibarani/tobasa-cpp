#pragma once

#include <memory>
#include <functional>
#include <tobasa/json.h>
#include <tobasahttp/response.h>

namespace tbs {
namespace http {

/** \addtogroup WEB
 * @{
 */

class Result;
class Context;

using ResultPtr = std::shared_ptr<Result>;

/// Functor to create a result object
using ResultBuilder = std::function<std::shared_ptr<Result>(const std::shared_ptr<Context>&, StatusCode, const std::string&)>;

/// Functor to build Result's content
using ResultContentBuilder = std::function<std::string(std::shared_ptr<Result> result)>;


/**
 * @class Result
 * @brief Request/route handler result base class.
 * 
 * The Result class encapsulates the content, content type, HTTP status, and other metadata
 * related to an HTTP response. It provides methods to manipulate these properties and to
 * apply them to an actual HTTP response object.
 */
class Result
   : public std::enable_shared_from_this<Result>
{
protected:
   std::string _content;
   std::string _contentType {"text/plain"};
   HttpStatus  _httpStatus  { StatusCode::OK };
   std::string _redirectPath;
   Json        _metadata;
   bool        _ignoreContentBuilder = false;
   std::string _contentDisposition;
   bool        _enableCompression = false;

public:
   Result();
   Result(const std::string& content, const std::string& contentType);
   virtual ~Result();

   void       redirect(const std::string& path);
   bool       redirected();

   void       statusCode(StatusCode statusCode);
   void       httpStatus(HttpStatus status);
   HttpStatus httpStatus();

   std::string& content() { return _content; }

   std::string contentDisposition();
   void contentDisposition(const std::string& value);

   std::string contentType();
   void contentType(const std::string& contentType);
 
   Json& metadata();
   void metadata(const Json& metadata);

   virtual std::string className() { return "http::Result"; }

   virtual std::string buildContent() { return {}; }

   virtual void content(const std::string& content);

   /** 
    * Apply values to http response
    * Set response's content, http status, content type to http response
    * \param response http response object
    * \param contentBuilder functor to build response's content
    */
   virtual void toResponse(std::shared_ptr<Response> response, ResultContentBuilder contentBuilder=nullptr);

   void ignoreContentBuilder(bool val=true);
};




template <class ResultType, typename... Params>
[[nodiscard]]
std::shared_ptr<Result> makeResult(Params&&... args)
{
   return std::static_pointer_cast<Result>(
      std::make_shared<ResultType>(std::forward<Params>(args)...)
   );
}

template <typename... Params>
[[nodiscard]]
std::shared_ptr<Result> makeResult(Params&&... args)
{
   return std::make_shared<Result>(std::forward<Params>(args)...);
}


/**
 * Status Page Result with html content
 */
class StatusResult : public Result
{
private:
   std::string _message;

public:
   StatusResult(HttpStatus status);
   StatusResult(const std::string& message={});
   StatusResult(StatusCode statusCode, const std::string& message={});
   
   virtual std::string className() { return "http::StatusResult"; }
   virtual std::string buildContent();
   virtual std::string& message() { return _message; }
};

template <typename... Params>
[[nodiscard]]
std::shared_ptr<http::Result>
statusResultHtml(Params&&... args)
{
   return std::static_pointer_cast<http::Result>(
      std::make_shared<StatusResult>(std::forward<Params>(args)...)
   );
}


/**
 * File Result
 */
class FileResult : public Result
{
private:
   std::string _filePath;

public:
   FileResult(const std::string& filePath);
   FileResult(const std::string& filePath, const std::string& contentType);
   virtual std::string className() { return "http::FileResult"; }
   virtual void toResponse(std::shared_ptr<Response> response, ResultContentBuilder contentBuilder=nullptr);
};

template <typename... Params>
[[nodiscard]]
std::shared_ptr<http::Result>
fileResult(Params&&... args)
{
   return std::static_pointer_cast<http::Result>(
      std::make_shared<FileResult>(std::forward<Params>(args)...)
   );
}


/**
 * Raw Bytes Result
 */
class RawBytesResult : public Result
{
private:
   nonstd::span<const unsigned char> _rawBytes;

public:
   RawBytesResult(const nonstd::span<const unsigned char>& rawBytes);
   RawBytesResult(const nonstd::span<const unsigned char>& rawBytes, const std::string& contentType);
   virtual std::string className() { return "http::RawBytesResult"; }
   virtual void toResponse(std::shared_ptr<Response> response, ResultContentBuilder contentBuilder=nullptr);
};

template <typename... Params>
[[nodiscard]]
std::shared_ptr<http::Result>
rawBytesResult(Params&&... args)
{
   return std::static_pointer_cast<http::Result>(
      std::make_shared<RawBytesResult>(std::forward<Params>(args)...)
   );
}

/** @}*/

} // namespace http
} // namespace tbs