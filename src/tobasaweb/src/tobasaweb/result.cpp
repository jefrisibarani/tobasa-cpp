#include <iostream>
#include <tobasa/path.h>
#include <tobasa/util_string.h>
#include <tobasahttp/server/status_page.h>
#include <tobasahttp/mimetypes.h>
#include "tobasaweb/result.h"

namespace tbs {
namespace http {

Result::Result() {}

Result::Result(const std::string& content, const std::string& contentType)
   : _content     {content}
   , _contentType {contentType}
{}

Result::~Result() {}

void Result::redirect(const std::string& path) { _redirectPath = path; }

bool Result::redirected() { return _redirectPath.length() > 0 ; }

void Result::statusCode(StatusCode statusCode) { _httpStatus.code(statusCode); }

HttpStatus Result::httpStatus() { return _httpStatus; }

void Result::httpStatus(HttpStatus status)  { _httpStatus = status; }

std::string Result::contentType()  {  return _contentType;  }

std::string Result::contentDisposition() { return _contentDisposition; }

void Result::content(const std::string& content) { _content = content; }

void Result::contentType(const std::string& contentType) { _contentType = contentType; }

void Result::contentDisposition(const std::string& value) { _contentDisposition = value; }

void Result::metadata(const Json& metadata) { _metadata = metadata; }

Json& Result::metadata() { return _metadata; }

void Result::ignoreContentBuilder(bool val) { _ignoreContentBuilder = val; }

void Result::toResponse(std::shared_ptr<Response> response, ResultContentBuilder contentBuilder)
{
   response->httpStatus( this->httpStatus());

   if ( redirected() )
   {
      response->redirect( _redirectPath );
      return;
   }

   if (contentBuilder && !_ignoreContentBuilder)
      _content = contentBuilder(shared_from_this());
   else
   {
      auto content = this->buildContent();
      if (!content.empty())
         _content = std::move(content);
   }

   // Move _content to http response
   response->content(std::move(_content));

   response->setHeaderContentType(this->contentType());

   if ( ! this->contentDisposition().empty() )
      response->addHeader("Content-Disposition", this->contentDisposition());
}



StatusResult::StatusResult(HttpStatus status)
   : Result("", "text/html")
{
   _httpStatus = status;
}

StatusResult::StatusResult(const std::string& message)
   : Result("", "text/html")
{
   _httpStatus.code(StatusCode::OK);
   _message = message;
}

StatusResult::StatusResult(StatusCode statusCode, const std::string& message)
   : Result("", "text/html")
{
   _httpStatus.code(statusCode);
   _message = message;
}

std::string StatusResult::buildContent()
{
   return http::statusPageHtml(_httpStatus, _message);
}


FileResult::FileResult(const std::string& filePath)
   : _filePath {filePath}
{
   _ignoreContentBuilder = true;

   std::string ext = path::fileExtension(_filePath);
   util::strLower(ext);
   _contentType = http::mimetypes::fromExtension(ext);
}

FileResult::FileResult(const std::string& filePath, const std::string& contentType)
   : _filePath {filePath}
{
   _ignoreContentBuilder = true;

   _contentType = contentType;
   if (_contentType.empty())
   {
      std::string ext = path::fileExtension(_filePath);
      util::strLower(ext);
      _contentType = http::mimetypes::fromExtension(ext);
   }
}

void FileResult::toResponse(std::shared_ptr<Response> response, ResultContentBuilder contentBuilder)
{
   response->httpStatus( this->httpStatus());
   response->setHeaderContentType(this->contentType());

   if ( ! this->contentDisposition().empty() )
      response->addHeader("Content-Disposition", this->contentDisposition());

   // set response to get content from a file
   response->fileContent(_filePath);
}


RawBytesResult::RawBytesResult(const nonstd::span<const unsigned char>& rawBytes)
   : _rawBytes {rawBytes}
{
   _ignoreContentBuilder = true;

   // std::string ext = path::fileExtension(_filePath);
   // util::strLower(ext);
   // _contentType = http::mimetypes::fromExtension(ext);
}

RawBytesResult::RawBytesResult(const nonstd::span<const unsigned char>& rawBytes, const std::string& contentType)
   : _rawBytes {rawBytes}
{
   _ignoreContentBuilder = true;

   _contentType = contentType;
   if (_contentType.empty())
   {
      // std::string ext = path::fileExtension(_filePath);
      // util::strLower(ext);
      // _contentType = http::mimetypes::fromExtension(ext);
   }
}

void RawBytesResult::toResponse(std::shared_ptr<Response> response, ResultContentBuilder contentBuilder)
{
   response->httpStatus( this->httpStatus());
   response->setHeaderContentType(this->contentType());

   if ( ! this->contentDisposition().empty() )
      response->addHeader("Content-Disposition", this->contentDisposition());

   // set response to get content from a file
   response->rawBytesContent(_rawBytes);
}

} // namespace http
} // namespace tbs