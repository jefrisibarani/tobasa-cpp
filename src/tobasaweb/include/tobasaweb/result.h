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
 * \class Result
 * \brief Request/route handler result base class.
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




/// Create a Result of the specified type, passing arguments to its constructor.
template <class ResultType, typename... Params>
[[nodiscard]]
std::shared_ptr<Result> makeResult(Params&&... args)
{
   return std::static_pointer_cast<Result>(
      std::make_shared<ResultType>(std::forward<Params>(args)...)
   );
}

/// Create a base Result, passing arguments to its constructor.
template <typename... Params>
[[nodiscard]]
std::shared_ptr<Result> makeResult(Params&&... args)
{
   return std::make_shared<Result>(std::forward<Params>(args)...);
}


/**
 * A result that returns an HTML status page.
 * The page shows the HTTP status and an optional message.
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
 * A result that sends a file.
 * The content type is inferred from the file name unless supplied.
 * Byte-range requests are disabled by default.
 */
class FileResult : public Result
{
private:
   std::string _filePath;
   bool        _enableFileRange = false;

public:
   static const bool EnableFileRangeResponse = true;

   FileResult(const std::string& filePath, bool enableFileRange=false);
   FileResult(const std::string& filePath, const std::string& contentType, bool enableFileRange=false);
   virtual std::string className() { return "http::FileResult"; }
   virtual void toResponse(std::shared_ptr<Response> response, ResultContentBuilder contentBuilder=nullptr);
};


/// Create a FileResult that sends a file. The content type is inferred unless supplied.
/// Byte-range requests are disabled unless the optional flag is true.
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
 * A result that sends a byte span as the response body.
 * An optional content type can be supplied.
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

/// Create a RawBytesResult that sends the supplied bytes. A content type may be provided.
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