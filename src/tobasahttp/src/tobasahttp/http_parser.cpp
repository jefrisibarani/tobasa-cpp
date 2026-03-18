#include <cassert>
#include <string>
#include <map>
#include <iostream>
#include <vector>
#include <tobasa/format.h>
#include <tobasa/util.h>
#include <tobasa/util_string.h>
#include "tobasahttp/exception.h"
#include "tobasahttp/rule.h"
#include "tobasahttp/http_parser.h"
#include "tobasahttp/multipart_parser.h"

namespace tbs {
namespace http {
namespace parser {

const int MAX_REQUEST_TARGET_LENGTH = 1024*4; 

// StartLineContext
// -------------------------------------------------------
bool StartLineContext::done()
{
   return gotCR && gotLF;
}

bool StartLineContext::valid()
{
   return (gotCR && gotLF && !data.empty());
}

bool StartLineContext::consume(char ch)
{
   if ( rule::isVchar(ch) )
   {
      data.append(1, ch);
      return true;
   }

   return false;
}


// HeadersContext
// -------------------------------------------------------
bool Line::valid()
{
   return ( !fieldName.empty() && !fieldValue.empty() );
}

bool HeadersContext::LineContext::done()
{
   // note: https://www.w3.org/Protocols/HTTP/1.0/spec.html#Tolerant
   // are we tolerant?
   //return foundLF;
   return gotCR && gotLF;
}

bool HeadersContext::LineContext::valid()
{
   return (gotCR && gotLF && !nameBuff.empty() && !valueBuff.empty());
}

bool HeadersContext::LineContext::consume(char ch)
{
   // No whitespace is allowed between the header field-name and colon
   if ( ch == rule::COLON && isspace(prevChar) && ! nameBuff.empty() )
      return false;
   else if ( ch == rule::COLON && ! nameBuff.empty() && valueBuff.empty() )
   {
      gotCOLON = true;
      return true;
   }

   // Collect field name
   if ( ! gotCOLON )
   {
      if ( (rule::isDigit(ch) || rule::isAlpha(ch)) || !rule::isDelimiter(ch) )
      {
         nameBuff.append(1, ch);
         return true;
      }
   }
   else // colon found
   {
      // Collect value field, ignore OWS after COLON
      if ( valueBuff.empty() )
      {
         // ignore white space after colon
         if ( !isspace(ch) )
            valueBuff.append(1,ch);
      }
      else
      {
         if ( !rule::isCtl(ch) )
            valueBuff.append(1,ch);
      }

      return true;
   }

   return false;
}
  
void HeadersContext::startNewLine()
{
   lineCtx = {};
}

bool HeadersContext::done()
{
   // note: https://www.w3.org/Protocols/HTTP/1.0/spec.html#Tolerant
   // are we tolerant?
   return gotCR && gotLF;
}

void HeadersContext::saveLine()
{
   lines.emplace_back(
      Line { std::move(lineCtx.nameBuff),
             std::move(lineCtx.valueBuff) } );

   totalLine++;
}

Line HeadersContext::findLine(const std::string& name)
{
   if ( done() )
   {
      for ( auto line: lines )
      {
         if (util::toLower(line.fieldName) == util::toLower(name))
            return line;
      }
   }

   return Line{};
}

bool HeadersContext::lineExists(const std::string& name, const std::string& value)
{
   if ( done() )
   {
      for ( auto line: lines )
      {
         if (util::toLower(line.fieldName) == util::toLower(name) &&
            line.fieldValue == value)
            return true;
      }
   }
   return false;
}


// ChunkedContext
// -------------------------------------------------------
void ChunkedContext::prepareForNextChunk()
{
   sizeCR     = false;
   sizeLF     = false;
   dataCR     = false;
   dataLF     = false;
   lastCR     = false;
   lastLF     = false;
   finalChunk = false;
   dataCount  = 0;
   rawSize    = {};
   hasTrailer = {false};
   headersCtx = {};
}


// Parser
// -------------------------------------------------------

Parser::Parser(Type parserType, std::vector<char>& buffer, 
              size_t maxHeaderSize, bool enableMultipart, const std::string& temporaryDir)
   : _type            {parserType}
   , _readBuffer      {buffer}
   , _maxHeaderSize   {maxHeaderSize}
   , _enableMultipart {enableMultipart}
   , _temporaryDir    {temporaryDir}
{
   if (_type == Type::REQUEST)
      _typeString = "request";
   else
      _typeString = "response";

   _headersCtx.maxHeaderSize = maxHeaderSize;
}

Parser::~Parser() {}

void Parser::type(Type parserType)
{
   _type = parserType;
}

bool Parser::done() const
{
   return _contentDone && _headersDone;
}

bool Parser::hasContent() const
{
   return ( _contentLength > 0 && _content.size() > 0 );
}

bool Parser::headersDone() const
{
   return _headersDone;
}

bool Parser::contentDone() const
{
   return _contentDone;
}

std::string Parser::method() const
{
   return _method;
}

std::string Parser::requestTarget() const
{
   return _requestTarget;
}

std::string Parser::requestLine() const
{
   return _requestLine;
}

uint16_t Parser::majorVersion() const
{
   return _majorVersion;
}

uint16_t Parser::minorVersion() const
{
   return _minorVersion;
}

Headers Parser::headers() const
{
   Headers headers;
   for( auto hdr: _headersCtx.lines ) {
      headers.add(std::move(hdr.fieldName), std::move(hdr.fieldValue));
   }

   return std::move(headers);
}

std::string Parser::content()
{
   return std::move(_content);
}

MultipartBodyUPtr Parser::multipartBody()
{
   return std::move(_multipartBody);
}

bool Parser::isReading() const
{
   return _isReading;
}

void Parser::isReading(bool value)
{
   _isReading = value;
}

HeadersContext& Parser::headerContext()
{
   return _headersCtx;
}

Field Parser::findHeader(const std::string& name)
{
   auto line = _headersCtx.findLine(name);
   if ( line.valid() )
      return Field( line.fieldName, line.fieldValue );

   return Field{};
}

Info Parser::parse(size_t bytesTransferred)
{
   return parseWithOwnParser(bytesTransferred);
}

uint16_t Parser::statusCode() const
{
   return _statusCode;
}

std::string Parser::statusMessage() const
{
   return _statusMessage;
}

void Parser::prepareForNextMessage()
{
   _isReading              = false;
   _contentDone            = false;
   _headersDone            = false;
   _hasMultipart           = false;
   _hasChunkedEncoding     = false;
   _hasContentLength       = false;
   _totalParsedBytes       = 0;

   _headersCtx             = {};
   _headersCtx.maxHeaderSize = _maxHeaderSize;
   _majorVersion           = 1;
   _minorVersion           = 1;
   _content                = {};
   _contentLength          = 0;
   _statusCode             = 0;
   _statusMessage          = {};
   _method                 = {};
   _requestTarget          = {};
   _requestLine            = {};

   _startLineContext       = {};
   _chunkedCtx             = {};
   _multipartParser        = {nullptr};
   _multipartBody          = {nullptr};
   _parsingId              = 0;
}

Info Parser::parseWithOwnParser(size_t bytesTransferred)
{
   if (!_headersDone)
   {
      size_t bytesRead  = 0;
      size_t currentIdx = 0;

      for ( auto it = _readBuffer.begin();
                 it < _readBuffer.begin() + bytesTransferred; ++it )
      {
         ++bytesRead;
         currentIdx = it - _readBuffer.begin();
         char ch = *it;

         if (!_startLineContext.done())
         {
            // -------------------------------------------------------
            // line starts with white space, stop here
            if ( _startLineContext.data.empty() && isspace(ch) ) {
               return withError("Invalid Start line", currentIdx);
            }
            else if ( ch != rule::CR && ch != rule::LF && !_startLineContext.done() )
            {
               // incoming char is not CR/LF, get the char
               bool ok = _startLineContext.consume(ch);
               _startLineContext.prevChar = ch;
               if (!ok)
                  return withError("Invalid character for start line", currentIdx);
               else
                  continue;
            }
            else if ( ch == rule::CR && !_startLineContext.gotCR && !_startLineContext.data.empty() )
            {
               // CR found, expect LF as next char, skip this loop
               _startLineContext.gotCR = true;
               continue;
            }
            else if ( ch == rule::LF && _startLineContext.gotCR && !_startLineContext.gotLF && !_startLineContext.data.empty() )
            {
               _startLineContext.gotLF = true;
               
               std::string_view startLine{_startLineContext.data};
               Info info;
               if ( _type == Type::REQUEST )
                  info = parseRequestStartLine(startLine);
               else
                  info = parseResponseStartLine(startLine);

               if ( info.success() )
                  continue;
               else
               {
                  info.lastIndex(currentIdx);
                  return std::move(info);
               }
            }
            else
               return withError("Invalid data buffer for HTTP Headers", currentIdx);
            // -------------------------------------------------------
         }
         else
         {
            if (! _headersCtx.done())
            {
               bytesRead -= 1;
               auto info = retrieveHeaders(_headersCtx, currentIdx, bytesTransferred-currentIdx);
               it = _readBuffer.begin() + info.lastIndex();
               bytesRead += info.bytesRead();
               currentIdx = it - _readBuffer.begin();  // keep track current index
               if (info.success())
               {
                  if (_headersCtx.done())  // all headers collected, do some processing, passing next index
                  {  
                     _totalParsedBytes += bytesRead;
                     return processHeader(bytesTransferred, currentIdx);
                  }
                  else
                     continue;
               }
               else 
               {
                  _totalParsedBytes += bytesRead;
                  return info;
               }
            }
         }
      }

      _totalParsedBytes += bytesRead;
      return withSuccess(currentIdx, bytesRead);  // headers not completed, need more data
   }
   else
   {
      // headers done, body in progress
      auto info = checkNeedProcessBody(bytesTransferred, 0);
      _totalParsedBytes += info.bytesRead();
      return info;
   }
}

Info Parser::parseResponseStartLine(std::string_view rawtext)
{
   // minimal response : HTTP/1.1 200 OK  : 15 char
   if ( rawtext.length() < 15 )
      return withError("Invalid start-line: min-length");

   auto pos = rawtext.find_first_of(rule::SP);
   if ( pos==std::string_view::npos )
      return withError("Invalid start-line: no sp");
   else
   {
      auto version = rawtext.substr(0, pos);
      if ( version.length() !=8 && "HTTP/" != version.substr(0, 5) )
         return withError("Invalid start-line http protocol");
      else
      {
         version.remove_prefix(5);
         auto major = version.at(0);
         auto dot   = version.at(1);
         auto minor = version.at(2);

         if ( dot != '.' )
            return withError("Invalid start-line: version format");
         else if (major != '1')
            return withError("Invalid start-line: major ver");
         else if (minor != '0' && minor != '1')
            return withError("Invalid start-line: minor ver");
         else
         {
            _majorVersion  = 1;
            _minorVersion  = minor == '1' ? 1 : 0;
         }

         rawtext.remove_prefix(pos+1);
         pos = rawtext.find_first_of(rule::SP);
         if ( pos==std::string_view::npos )
            return withError("Start line: no space after version");
         else
         {
            auto statusCode = rawtext.substr(0, pos);
            if ( statusCode.length() != 3 )
               return withError("Start line: status code");
            else
            {
               uint16_t intCode = 0;
               for ( int i=0; i<3; ++i )
               {
                  auto chr = statusCode.at(i);
                  if ( (chr >= 48  && chr <= 57) )
                     intCode = intCode * 10 + chr - '0';
                  else
                     return withError("Start line: status code");
               }
               _statusCode = intCode;

               rawtext.remove_prefix(4);
               _statusMessage = std::string{rawtext};
            }
         }
      }
   }
   return withSuccess();
}

Info Parser::parseRequestStartLine(std::string_view rawtext)
{
   // https://datatracker.ietf.org/doc/html/rfc7230#section-2.5
   // request-line   = method SP request-target SP HTTP-version CRLF

   // minimal request  : GET / HTTP/1.1   : 14 chars
   if ( rawtext.length() < 14 )
      return withError("Invalid start-line: min-length");

   auto pos = rawtext.find_first_of(rule::SP);
   if ( pos == std::string_view::npos )
      return withError("Invalid start-line: no sp");
   else
   {
      using namespace std::literals;

      // https://datatracker.ietf.org/doc/html/rfc7231#section-4
      // common methods : GET, POST, PUT, DELETE, HEAD, OPTIONS, CONNECT, TRACE
      _method = std::string {rawtext.substr(0, pos)};

      if ( onValidateRequestMethod )
      {
         HttpStatus status;
         bool allow = onValidateRequestMethod(_method, status);
         if (! allow)
            return { false, "Method not allowed", 0, status, 0 };
      }

      rawtext.remove_prefix(pos+1);
      pos = rawtext.find_first_of(rule::SP);
      if ( pos==std::string_view::npos )
         return withError("Invalid start-line: no sp");
      else
      {
         auto reqUri = rawtext.substr(0,pos);
         _requestTarget = std::string{ reqUri };

         if (_requestTarget.length() > MAX_REQUEST_TARGET_LENGTH)
         {
            // 414 URI Too Long
            // https://datatracker.ietf.org/doc/html/rfc7231#section-6.5.12
            // https://datatracker.ietf.org/doc/html/rfc7230#section-3.1.1
            return { false, "Http request uri too long", 0,  HttpStatus(StatusCode::URI_TOO_LONG), 0 };
         }

         auto ch = _requestTarget.front();
         if ( ch == rule::SP || ch == rule::QUESTION || ch == rule::PERCENT ) {
            return withError("Invalid request target");
         }

         rawtext.remove_prefix(pos+1);
         auto version = rawtext;
         if ( version.length() !=8 && "HTTP/" != version.substr(0, 5) )
            return withError("Invalid start-line http protocol");
         else
         {
            version.remove_prefix(5);
            auto major = version.at(0);
            auto dot   = version.at(1);
            auto minor = version.at(2);

            if ( dot != '.' )
               return withError("Invalid start-line: version format");
            else if (major != '1')
               return withError("Invalid start-line: major ver");
            else if (minor != '0' && minor != '1')
               return withError("Invalid start-line: minor ver");
            else
            {
               _majorVersion = 1;
               _minorVersion = minor == '1' ? 1 : 0;
               _requestLine  = _method + " " + _requestTarget + " HTTP/1." + std::to_string(_minorVersion);
            }
         }
      }
   }

   return withSuccess();
}

Info Parser::processHeader(size_t bytesTransferred, size_t lastIndex)
{
   _headersDone = true;
   // All headers collected, now we can find body informations
   if ( onEventDone )
      onEventDone("headers");

   if ( onValidateHeaders )
   {
      HttpStatus status;
      bool allow = onValidateHeaders(*this, status);
      if (! allow)
         return { false, "Invalid headers", 0, status, lastIndex };
   }

   // https://datatracker.ietf.org/doc/html/rfc7230#section-3.3.2

   // Read content length from header
   auto contentLen = _headersCtx.findLine("Content-Length").fieldValue;
   if ( !contentLen.empty() )
   {
      _contentLength    = std::stoull(contentLen);
      _hasContentLength = true;
   }

   // Check for transfer encoding - chunked
   _hasChunkedEncoding = _headersCtx.lineExists("Transfer-Encoding", "chunked");
   
   // Check for multipart
   // https://httpwg.org/specs/rfc7231.html#header.content-type
   // If we have multipart body, init _multipartCtx's boundary
   auto header = _headersCtx.findLine("Content-Type");
   if ( header.valid())
   {
      MediaType media;
      media.parse(header.fieldValue);
      if ( media.valid() && util::toLower(media.fullType()) == "multipart/form-data" )
      {
         if (_multipartParser == nullptr) {
            _multipartParser = std::make_unique<MultipartParser>(_temporaryDir);
         }
         else {
            _multipartParser->prepareForNextMessage();
         }
         // set id for debugging
         _multipartParser->id( tbsfmt::format("C({})_R({})_Sync", _connId, _parsingId ));

         _multipartParser->contentLength(_contentLength);

         auto boundary = media.find("boundary");
         if ( boundary && boundary->valid() && _multipartParser->applyBoundary(boundary->value()) )
         {
            _hasMultipart = true;
            if (_enableMultipart) 
               _multipartBody = std::make_unique<MultipartBody>();
            else
               _multipartParser.reset();
         }
         else
            return withError("Invalid multipart boundary", lastIndex);
      }
   }

   if ( _hasChunkedEncoding && _hasContentLength )
      return withError("Found content length and chunked transfer", lastIndex);

   bool hasExpect100 = _headersCtx.lineExists("Expect", "100-continue");
   if ( hasExpect100 ) {
      return { true, "expect 100-continue", 0, HttpStatus(StatusCode::CONTINUE), lastIndex };
   }

   if (_method == "GET" || _method == "HEAD" || _method == "TRACE")
      _contentDone = true;
   else if ( _contentLength == 0 && !_hasChunkedEncoding && !_hasMultipart ) {
      _contentDone = true; // no body in http message.
   }

   if (_contentDone)
   {
      if ( onEventDone )
         onEventDone("body");

      return withSuccess(lastIndex);
   }

   auto info = checkNeedProcessBody(bytesTransferred, lastIndex+1);
   _totalParsedBytes += info.bytesRead();
   return info;
}

Parser::BufferInfo Parser::ComputeBufferInfo(size_t bytesTransferred, size_t bodyStartPosition)
{
   size_t readBufferSize   = _readBuffer.size();
   size_t readableBuffSize = ( bytesTransferred <= readBufferSize ? bytesTransferred : readBufferSize );
   size_t dataStart = 0;
   size_t totalData = 0;

   if ( bodyStartPosition > 0 )
   {
      if (bodyStartPosition <= readBufferSize )
      {
         dataStart = bodyStartPosition;
         totalData = readableBuffSize - dataStart;
      } 
      else if (bodyStartPosition % readBufferSize == 0)
      {
         dataStart = readBufferSize ;
         totalData = 0;
      }
      else
      {
         // happen when the size of HTTP request headers bigger than readBufferSize
         auto remainder = bodyStartPosition % readBufferSize;
         dataStart = remainder ;
         totalData = readableBuffSize - dataStart;
      }
   }
   else 
   {
      // more data is coming
      dataStart = 0;
      totalData = readableBuffSize;
   }

   BufferInfo bufInfo;
   bufInfo.dataStart = dataStart;
   bufInfo.totalData = totalData;

   return bufInfo;
}

Info Parser::checkNeedProcessBody(size_t bytesTransferred, size_t bodyStartPosition)
{
   if ( !_enableMultipart)
   {
      if ( _hasMultipart && _hasChunkedEncoding && !_hasContentLength )
      {
         // Multipart with Chunked Transfer Encoding
         if (processBodyStarter == nullptr)
         {
            // will be called by MultipartBodyReader to initiate this parser's processBody()
            processBodyStarter = [&, transferred = bytesTransferred, start = bodyStartPosition]() 
            {
               return processBody(transferred, start);
            };

            // Return back control to ServerConnection to process/retrieve data
            size_t lastIndex = bodyStartPosition >  0 ? bodyStartPosition-1 : 0;
            size_t bytesRead = bodyStartPosition >= 0 ? bodyStartPosition   : 0;
            return {true, "process_multipart_chunked", lastIndex, {}, bytesRead};
         }
      }

      else if ( _hasMultipart && _hasContentLength && _contentLength > 0 )
      {
         // Normal Multipart
         // Return back control to ServerConnection to process/retrieve data
         size_t lastIndex = bodyStartPosition >  0 ? bodyStartPosition-1 : 0;
         size_t bytesRead = bodyStartPosition >= 0 ? bodyStartPosition   : 0;
         return {true, "process_multipart", lastIndex, {}, bytesRead};
      }
   }

   // Default
   return processBody(bytesTransferred, bodyStartPosition);
}

Info Parser::processBody(size_t bytesTransferred, size_t bodyStartPosition)
{
   try
   {
      auto bufInfo = ComputeBufferInfo(bytesTransferred, bodyStartPosition);
      size_t dataStart = bufInfo.dataStart;
      size_t totalData = bufInfo.totalData;

      // Deal with Mutipart Body with chunked transfer-encoding
      if ( _hasMultipart && _hasChunkedEncoding && !_hasContentLength)
      {
         return retrieveChunkedBody(dataStart, totalData);
      }
      // Deal with Multipart body
      else if ( _enableMultipart && _hasMultipart && _hasContentLength && _contentLength > 0 )
      {
         // Parse multipart body
         return retrieveMultipartBody(dataStart, totalData);
      }
      // Deal with transfer encoding
      else if ( _hasChunkedEncoding && !_hasContentLength )
      {
         return retrieveChunkedBody(dataStart, totalData);
      }
      else if ( _hasContentLength && _contentLength > 0 )
      {
         return retrieveBody(dataStart, totalData);
      }
      else
      {
         // either content length header not found, or found but the value set to 0.
         // we must stop parsing/building content
         _contentDone = true;
         if ( onEventDone )
            onEventDone("body");

         return withSuccess();
      }
   }
   catch (const std::exception& ex)
   {
      return { false, ex.what(), 0, HttpStatus(StatusCode::INTERNAL_SERVER_ERROR), 0 };
   }  
}

Info Parser::retrieveMultipartBody(size_t dataStart, size_t totalData)
{
   char* data = nullptr;
   data = _readBuffer.data() + dataStart;

   auto info = _multipartParser->parse(reinterpret_cast<const uint8_t *>(data), totalData);
   _contentDone = _multipartParser->done();
   if (_contentDone)
   {
      _multipartBody = _multipartParser->multipartBody();
      if ( onEventDone )
         onEventDone("multipart");
   }

   return info;
}

Info Parser::retrieveBody(size_t dataStart, size_t totalData)
{
   size_t dataEnd = dataStart + totalData; 

   char* buffer = nullptr;
   buffer = _readBuffer.data() + dataStart ;
   _content.append(buffer, totalData);
   if ( _content.size() == _contentLength )
   {
      _contentDone = true;

      if ( onEventDone )
         onEventDone("body");

      return withSuccess(dataEnd,totalData);
   }
   return withSuccess(dataEnd,totalData);
}

Info Parser::retrieveChunkedBody(size_t dataStart, size_t totalData)
{
   size_t bytesRead  = 0;
   size_t currentIdx = 0;
   size_t dataEnd    = dataStart + totalData;

   for (auto it = _readBuffer.begin() + dataStart; 
             it < _readBuffer.begin() + dataEnd; ++it)
   {
      ++bytesRead;
      currentIdx = it - _readBuffer.begin();
      char ch = *it;
      // Parse chunked data
      if ( !_chunkedCtx.sizeDone())
      {
         if ( rule::isHex(ch) )
         {
            _chunkedCtx.rawSize.append(1,ch);
            continue;
         }
         else
         {
            if ( ch == rule::CR && !_chunkedCtx.sizeCR )
            {
               _chunkedCtx.sizeCR = true;
               continue;
            }
            else if ( ch == rule::LF && _chunkedCtx.sizeCR )
            {
               _chunkedCtx.sizeLF = true;
               if ( _chunkedCtx.dataSize() == 0 )   // Final chunk
               {
                  auto hdr = _headersCtx.findLine("Trailer");
                  if (  hdr.valid() ) {
                     _chunkedCtx.hasTrailer = true;
                  }
                  _chunkedCtx.finalChunk = true;
               }

               continue;
            }
            else
               return withError("Malformed input after size", currentIdx);
         }
      }
      else if ( _chunkedCtx.sizeDone() && ! _chunkedCtx.dataDone() )
      {
         if ( _chunkedCtx.finalChunk && !_chunkedCtx.hasTrailer)
         {
            if ( ch == rule::CR && !_chunkedCtx.lastCR)
            {
               _chunkedCtx.lastCR = true;
               continue;
            }
            else if ( ch == rule::LF && !_chunkedCtx.lastLF)
            {
               _chunkedCtx.lastLF = true;
               _contentDone = true;

               if ( onEventDone )
                  onEventDone("body-chunked");

               return withSuccess(currentIdx,bytesRead);
            }
            else
               return withError("Malformed input after chunked final chunk", currentIdx);
         }
         else if ( _chunkedCtx.finalChunk && _chunkedCtx.hasTrailer)
         {
            if ( !_chunkedCtx.headersCtx.done() )
            {
               auto info = retrieveHeaders(_chunkedCtx.headersCtx, currentIdx, dataEnd-currentIdx);
               it = _readBuffer.begin() + info.lastIndex();

               auto bytesReadX = info.lastIndex()-currentIdx;
               bytesRead += info.bytesRead();

               currentIdx = it - _readBuffer.begin();
               if (info.success())
               {
                  _contentDone = true;

                  if ( onEventDone )
                     onEventDone("body-chunked");

                  return withSuccess(currentIdx,bytesRead); // Return now with current index, 
               }
               else
                  return withError(info.message(), currentIdx);
            }
         }
         else if ( _chunkedCtx.dataCount < _chunkedCtx.dataSize() )
         {
            size_t chunkLeft  = _chunkedCtx.dataSize() - _chunkedCtx.dataCount;
            size_t chunkStart = currentIdx;
            size_t chunkEnd   = dataEnd;
            if (currentIdx + chunkLeft <= dataEnd)
               chunkEnd = currentIdx + chunkLeft;
               
            size_t chunkLen = chunkEnd - chunkStart;
            char* bfr = _readBuffer.data() + chunkStart;

            if (_hasMultipart && _enableMultipart)
            {
               // Multipart & Transfer-Encoding: chunked, has no content-length
               _multipartParser->chunkedMultipart(true);

               auto info = _multipartParser->parse(reinterpret_cast<const uint8_t *>(bfr), chunkLen);
               if (! info.success())
                  return withError(info.message(), currentIdx);
               
               if (_multipartParser->done())
               {
                  _multipartBody = _multipartParser->multipartBody();
                  if ( onEventDone )
                     onEventDone("multipart");
               }
            }
            else if (_hasMultipart && !_enableMultipart)
            {
               // Forward parsing to other party
               if( ! parseChunkedMultipartHandler )
                  return withError("Multipart parser is not available", currentIdx);

               auto info = parseChunkedMultipartHandler(reinterpret_cast<const uint8_t *>(bfr), chunkLen);
               if (!info.success())
                  return withError(info.message(), currentIdx);

               if ( info.message() == "multipart-done" && onEventDone)
                  onEventDone("multipart");
            }
            else
            {
               // just append to content
               _content.append(bfr, chunkLen);
            }

            _chunkedCtx.dataCount += chunkLen;

            it = _readBuffer.begin() + chunkEnd-1;
            currentIdx = it - _readBuffer.begin();
            bytesRead += currentIdx;
            continue;
            //_content.append(1, ch);
            //++_chunkedCtx.dataCount;
            //continue;
         }
         else
         {
            if ( ch == rule::CR && !_chunkedCtx.dataCR )
            {
               _chunkedCtx.dataCR = true;
               continue;
            }
            else if ( ch == rule::LF && _chunkedCtx.dataCR )
            {
               _chunkedCtx.dataLF = true;
               // this segment finished, prepare next segment
               _chunkedCtx.prepareForNextChunk();
               continue;
            }
            else
               return withError("Malformed input after data chunk", currentIdx);
         }
      }
      else
         return withError("Invalid data buffer for Chunked Body", currentIdx);
   }

   return withSuccess(currentIdx, bytesRead);
}

Info Parser::retrieveHeaders(HeadersContext& hdrCtx, size_t dataStart, size_t totalData)
{
   char* data = nullptr;
   data = _readBuffer.data();
   return parseHeaders(hdrCtx, reinterpret_cast<const uint8_t *>(data), dataStart, totalData);
}

Info withSuccess(size_t lastIndex, size_t bytesRead)
{
   return { true, std::string_view{}, lastIndex, HttpStatus{}, bytesRead };
}

Info withError(std::string_view message, size_t lastIndex, HttpStatus httpStatus, size_t bytesRead )
{
   return { false, message, lastIndex, httpStatus, bytesRead};
}

Info parseHeaders(HeadersContext& hdrCtx, const uint8_t *data, size_t dataStart, size_t totalData)
{
   size_t bytesRead  = 0;
   size_t lastIndex  = 0;
   size_t currentIdx = 0;
   size_t dataEnd    = dataStart + totalData;

   for (currentIdx = dataStart; currentIdx < dataEnd; ++currentIdx)
   {
      ++bytesRead;
      lastIndex = currentIdx; // save current index. After this for-loop, currentIdx is equals dataEnd
      char ch = *(data + currentIdx);

      if ( !hdrCtx.done() )
      {
         if ( hdrCtx.lineCtx.done() )
         {
            if (ch != rule::CR && ch != rule::LF)
               hdrCtx.startNewLine();
            else
            {
               // found CR, almost reach end of headers
               if ( ch == rule::CR && !hdrCtx.gotLF )
               {
                  hdrCtx.gotCR = true;
                  continue;
               }
               else if ( ch == rule::LF && hdrCtx.gotCR )
               {
                  hdrCtx.gotLF = true;
                  // All headers collected, now we can find body informations
                  return withSuccess(currentIdx, bytesRead); // Return now with current index, 
               }
               else
                  return withError("Invalid header end", currentIdx);
            }
         }

         auto& line = hdrCtx.lineCtx;

         // line starts with white space, stop here
         if ( line.nameBuff.empty() && isspace(ch) )
            return withError("Invalid header start line", currentIdx);

         // incoming char is not CR/LF, get the char
         if ( ch != rule::CR && ch != rule::LF && !line.done() )
         {
            // incoming char is not CR/LF, get the char
            hdrCtx.consumedByte++;
            if ( hdrCtx.consumedByte > hdrCtx.maxHeaderSize ) {
               return withError("Header section too large", currentIdx, HttpStatus(StatusCode::REQUEST_HEADER_FIELDS_TOO_LARGE));
            }


            bool ok = line.consume(ch);
            line.prevChar = ch;
            if ( !ok )
               return withError("Invalid header format", currentIdx);
            else
               continue;
         }
         else if ( ch==rule::CR && !line.gotCR && !line.nameBuff.empty() )
         {
            // CR found, expect LF as next char, skip this loop
            line.gotCR = true;
            continue;
         }
         else if ( ch==rule::LF && line.gotCR && !line.gotLF && !line.nameBuff.empty() )
         {
            // LF found, this is the end of a header.
            line.gotLF = true;
            hdrCtx.saveLine();
            continue;
         }
         else
            return withError("Invalid data buffer for HTTP Headers - a", currentIdx);
      }
   }

   return withSuccess(lastIndex, bytesRead);
}

} // namespace parser
} // namespace http
} // namespace tbs