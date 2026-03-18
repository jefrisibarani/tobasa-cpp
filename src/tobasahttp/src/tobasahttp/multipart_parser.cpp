#include <random>
#include <filesystem>
#include <tobasa/nonstd_span.hpp>
#include <tobasa/util_string.h>
#include <tobasa/format.h>
#include <tobasa/crypt.h>
#include <tobasa/logger.h>
#include <tobasa/path.h>
#include "tobasahttp/multipart_parser.h"
#include "tobasahttp/rule.h"

namespace tbs {
namespace http {
namespace parser {

namespace fs = std::filesystem;

// MultipartContext
// -------------------------------------------------------

MultipartContext::MultipartContext()
{
   part = std::make_shared<MultipartBody::Part>();
   tailBuffer = {};
}

void MultipartContext::prepareForNextPart(const std::string& tempDir)
{
   bdryBuff       = {};
   firstBdry      = {false};
   bdryPostCR     = {false};
   bdryPostLF     = {false};
   lastBdry       = {false};
   body           = {};
   headersCtx     = {};
   bodyCompleted  = {false};
   part           = std::make_shared<MultipartBody::Part>();
   tailBuffer     = {};
   temporaryDir   = tempDir;
}

void MultipartContext::boundaryDone(bool done)
{
   bdryPostCR = true;
   bdryPostLF = true;
}

bool MultipartContext::applyBoundary(std::string_view rawBoundary)
{
   if ( ! validateBoundary(rawBoundary) )
      return false;

   boundary        = "--" + std::string{rawBoundary};
   bdryToken       = "\r\n" + boundary + "\r\n";
   bdryTokenEnd    = "\r\n" + boundary + "--\r\n";
   bdryTokenLen    = bdryToken.size();
   bdryTokenEndLen = bdryTokenEnd.size();

   return true;
}

bool MultipartContext::validateBoundary(std::string_view boundary)
{
   // https://www.rfc-editor.org/rfc/rfc2046#section-5.1.1
   if ( boundary.empty() )
      return false;

   if ( boundary.size() > 70 )
      return false;

   if ( boundary.back() == rule::SP )
      return false;

   using namespace std::literals;
   for ( auto c: boundary )
   {
      if ( rule::isDigit(c) || rule::isAlphaLow(c) || rule::isAlphaUp(c) ||
            ( "'()+_,-./:=? "sv.find_first_of(c) != std::string_view::npos ) )
      {
         return true;
      }
   }

   return false;
}

bool MultipartContext::boundaryDone()
{
   return bdryPostCR && bdryPostLF;
}

Headers MultipartContext::headers()
{
   Headers headers;
   for ( auto hdr: headersCtx.lines )
   {
      headers.add(std::move(hdr.fieldName), std::move(hdr.fieldValue));
   }
   return std::move(headers);
}

void MultipartContext::initializePart()
{
   Headers headers {};
   for ( auto hdr: headersCtx.lines )
   {
      if (util::toUpper(hdr.fieldName) == util::toUpper("Content-Type"))
      {
         MediaType mediaType;
         mediaType.parse(hdr.fieldValue);

         if ( mediaType.valid() )
            part->contentType = mediaType.fullType();
      }

      if ( util::toUpper(hdr.fieldName) == util::toUpper("Content-Disposition") )
      {
         Disposition disposition;
         disposition.parse(hdr.fieldValue);
         if( disposition.valid() && !disposition.empty())
         {
            if ( util::toUpper(disposition.name()) == util::toUpper("form-data") )
            {
               auto val = disposition.find("name");
               if ( val ) {
                  part->name = val->value();
               }

               auto valFile = disposition.find("filename");
               if ( valFile )
               {
                  part->fileName = valFile->value();
                  part->isFile   = true;
               }
            }
         }
      }

      headers.add(std::move(hdr.fieldName), std::move(hdr.fieldValue));
   }

   if (part->isFile)
   {
      if (!fs::exists(temporaryDir)) {
         fs::create_directories(temporaryDir);
      }

      // Try to preserve original extension
      std::string ext;
      if (!part->fileName.empty()) {
         auto pos = part->fileName.find_last_of('.');
         if (pos != std::string::npos)
            ext = part->fileName.substr(pos);
      }

      std::string fname = crypt::getCryptoRandomHex(10) + ext;
      part->location = (fs::path(temporaryDir) / fname).string();

      // Open persistent file stream
      part->ofs.open(part->location, std::ios::binary);
      if (!part->ofs.is_open()) {
            throw std::runtime_error("Failed to open file for multipart part");
      }
   }

   part->headers = std::move(headers);
   part->initialized = true;
}

void MultipartContext::savePart(MultipartBodyUPtr &partHolder)
{
   // One part finished, transfer the data from _multipartCtx to multipart content
   if (part->isFile && part->ofs.is_open()) 
   {
      part->ofs.flush();
      part->ofs.close();
   }

   partHolder->add(std::move(part));
}


// MultipartParser
// -------------------------------------------------------
MultipartParser::MultipartParser(const std::string& temporaryDir)
{
   std::string tmpDir = temporaryDir;
   if (temporaryDir.empty())
      tmpDir = path::temporaryDir();

   _tmpDir = tmpDir;
   _multipartCtx.temporaryDir = tmpDir;
   _multipartBody = std::make_unique<MultipartBody>();
}

MultipartParser::~MultipartParser() 
{}

MultipartBodyUPtr MultipartParser::multipartBody()
{
   return std::move(_multipartBody);
}


bool MultipartParser::applyBoundary(std::string_view rawBoundary)
{
   return _multipartCtx.applyBoundary(rawBoundary);
}

void MultipartParser::prepareForNextMessage()
{
   _contentDone      = false;
   _contentLength    = 0;
   _multipartCtx     = {};
   _multipartBody    = std::make_unique<MultipartBody>();
   
   _multipartCtx.temporaryDir = _tmpDir;
}

void MultipartParser::temporaryDir(const std::string& path)
{
   _tmpDir = path;
   _multipartCtx.temporaryDir = path;
}

//  Example
//
//  POST /upload HTTP/1.1\r\n
//  Host: localhost:8080\r\n
//  User-Agent: curl/8.0.1\r\n
//  Accept: */*\r\n
//  Content-Length: 356\r\n
//  Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n
//  \r\n
//  ------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n                               [ Fist Boundary]
//  Content-Disposition: form-data; name="userName"\r\n
//  \r\n
//  Jefri\r\n
//  ------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n                               [ Intermediate Boundary]
//  Content-Disposition: form-data; name="profileImage"; filename="b.png"\r\n
//  Content-Type: image/png\r\n
//  \r\n
//  <binary bytes of b.png>\r\n
//  ------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n                             [ Last Boundary]
//

//  data coming into this function is the first boundary untill the last boundary.
//  this function might be called more than once untill all parts saved.
//  We treat boundary as: \r\nBOUNDARY\r\n
//
//  If the boundary is split across incoming data,
//  we keep a tail buffer to store last  bdryTokenEndLen from current data.
//  When next data arrive, we prepend tail buffer into our working buffer.
//  then we find again the boundary
//
//  1. We find the first boundary, 
//  2. Parsing part's header.
//  3. Then we scan more bytes until we found another boundary(\r\nBOUNDARY\r\n).
//     If found, bytes before boundary is part's body. One part is completed
//    
//  If the boundary is Last Boundary, parsing completed.
//  If not, We repeat parsing part's header (step 2)
//
Info MultipartParser::parse(const uint8_t *data, size_t totalData)
{
   size_t lastIndex   = 0;
   size_t currentIdx  = 0;
   for (currentIdx = 0; currentIdx < totalData; ++currentIdx) 
   {
      lastIndex = currentIdx; // save current index. After this for-loop, currentIdx is equals dataEnd
      char ch = *(data + currentIdx);

      if ( ! _multipartCtx.bodyCompleted && _multipartCtx.headersCtx.done() )
      {
         // First Boundary already skipped, part's headers retrieved, part's body not yet.

         if (!_multipartCtx.partInitialized())
            _multipartCtx.initializePart();

         // Prepare our working buffer
         const uint8_t* workBuffer = data + currentIdx ;
         nonstd::span<const uint8_t> swBuffer;

         // check if we have tail buffer from previous data chunk
         size_t tailBufferSize = _multipartCtx.tailBuffer.size();
         std::vector<uint8_t> vwBuffer;
         if (tailBufferSize > 0)
         {
            //Logger::logD("MultipartParser has tail buffer. data size: {}", totalData);
            // we have boundary split accross data chunk
            // prepend tail buffer into vwBuffer
            vwBuffer.reserve(tailBufferSize + (totalData - currentIdx) );  // avoid reallocations
            vwBuffer.insert(vwBuffer.end(), _multipartCtx.tailBuffer.begin(), _multipartCtx.tailBuffer.end());
            vwBuffer.insert(vwBuffer.end(), workBuffer, workBuffer + (totalData - currentIdx) );

            swBuffer = nonstd::span<const uint8_t>(vwBuffer);
         }
         else
         {
            vwBuffer = std::vector<uint8_t>(workBuffer, workBuffer + (totalData - currentIdx));
            swBuffer = nonstd::span<const uint8_t>(workBuffer, (totalData - currentIdx) );
         }

         // Find Boundary
         auto bdryStart = std::search(swBuffer.begin(), swBuffer.end(),
                              _multipartCtx.bdryToken.begin(), _multipartCtx.bdryToken.end());
         
         if (bdryStart != swBuffer.end())
         {
            auto bodyEndPos = bdryStart - swBuffer.begin();
            nonstd::span<const uint8_t> partBody(swBuffer.begin(), swBuffer.begin() + bodyEndPos); // extract part's body from data buffer

            currentIdx = (lastIndex + bodyEndPos + _multipartCtx.bdryTokenLen-1) - tailBufferSize;

            auto nBytes = partBody.size();
            if (_multipartCtx.part->isFile)
               _multipartCtx.part->ofs.write( (char*) partBody.data(), nBytes );
            else
               _multipartCtx.part->body += std::string( (char*) partBody.data(), nBytes );

            _multipartCtx.bytesDone += nBytes + _multipartCtx.bdryTokenLen ;
            _multipartCtx.bodyCompleted = true;
            _multipartCtx.lastBdry = false;
            _multipartCtx.tailBuffer = {};
         }
         else if ( (bdryStart = std::search(swBuffer.begin(), swBuffer.end(),
                     _multipartCtx.bdryTokenEnd.begin(), _multipartCtx.bdryTokenEnd.end()) ) != swBuffer.end() )
         {
            // Final Boundary
            auto bodyEndPos = bdryStart - swBuffer.begin();
            nonstd::span<const uint8_t> partBody(swBuffer.begin(), swBuffer.begin() + bodyEndPos); // extract part's body from data buffer
            
            currentIdx = (lastIndex + bodyEndPos + _multipartCtx.bdryTokenEndLen-1) - tailBufferSize;

            auto nBytes = partBody.size();
            if (_multipartCtx.part->isFile)
               _multipartCtx.part->ofs.write( (char*) partBody.data(), nBytes);
            else
               _multipartCtx.part->body += std::string( (char*) partBody.data(), nBytes);
            
            _multipartCtx.bytesDone += nBytes + _multipartCtx.bdryTokenEndLen;
            _multipartCtx.bodyCompleted = true;
            _multipartCtx.lastBdry = true;
            _multipartCtx.tailBuffer = {}; 
         }
         else
         {
            if (swBuffer.size() >= _multipartCtx.bdryTokenEndLen)
            {
               // create tail buffer to to store last data from swBuffer as much as _multipartCtx.bdryTokenEndLen
               _multipartCtx.tailBuffer = std::vector<uint8_t>( (swBuffer.begin() + (swBuffer.size()-_multipartCtx.bdryTokenEndLen)), swBuffer.end());

               // write safe data from Vbuffer without last _multipartCtx.bdryTokenEndLen data
               nonstd::span<const uint8_t> partBody(swBuffer.begin(), (swBuffer.begin()+(swBuffer.size()-_multipartCtx.bdryTokenEndLen)) );

               auto nBytes = partBody.size();
               if (_multipartCtx.part->isFile)
                  _multipartCtx.part->ofs.write( (char*) partBody.data(), nBytes);
               else
                  _multipartCtx.part->body += std::string( (char*) partBody.data(), nBytes);

               currentIdx = totalData-1;
               _multipartCtx.bytesDone += nBytes;
               // need more data
               return withSuccess(currentIdx, _multipartCtx.bytesDone);
            }
            else
            {
               _multipartCtx.tailBuffer = std::vector<uint8_t>(swBuffer.begin(), swBuffer.end());
               currentIdx = totalData-1;
               // need more data
               return withSuccess(currentIdx, _multipartCtx.bytesDone);
            }
         }

         if (_chunkedMultipart)
         {
            // Multipart & Transfer-Encoding: chunked, has no content-length
            // We finish when we found the last boundary

            if ( _multipartCtx.bodyCompleted )
               _multipartCtx.savePart(_multipartBody);

            if (_multipartCtx.lastBdry)
               _contentDone = true;
         }
         else
         {
            // Normal Multipart with Content-Length
            // We finish when we received bytes equal to content-length

            if ( _multipartCtx.bytesDone == _contentLength && !_multipartCtx.lastBdry )
               return withError("Multipart closing boundary not found", currentIdx);

            // One part finished, transfer the data from _multipartCtx to multipart content
            if ( _multipartCtx.bodyCompleted )
               _multipartCtx.savePart(_multipartBody);

            if ( _multipartCtx.bytesDone == _contentLength && _multipartCtx.lastBdry )
            {
               // we finished here.
               _contentDone = true;
            }

            if ( _multipartCtx.bytesDone > _contentLength && _multipartCtx.lastBdry )
            {
               // we finished here.
               return withError("Received more data than content-length", currentIdx);
            }
         }

         continue;
      }

      if ( _multipartCtx.bodyCompleted )
      {
         // reset _multipartCtx, to begin parsing next part
         // a part has completed , next is another part's headers
         _multipartCtx.prepareForNextPart(_tmpDir);
         _multipartCtx.boundaryDone(true);
      }

      if ( _multipartCtx.boundaryDone() )
      {
         // Boundary found, get part's headers
         if ( !_multipartCtx.headersCtx.done() )
         {
            auto info = parseHeaders(_multipartCtx.headersCtx, data, currentIdx, totalData-currentIdx);
            _multipartCtx.bytesDone += info.lastIndex()-currentIdx + 1;

            // Update iterator position
            currentIdx = info.lastIndex();

            if (info.success())
              continue;
            else
               return info;
         }
      }
      else
      {
         // Read part's boundary
         // https://www.w3.org/Protocols/rfc1341/7_2_Multipart.html

         // TODO_JEFRI : part without headers (boundary followed by two CRLFs)

         // We only search for first boundary.
         // subsequent boundary will be removed when finalizing body

         if ( ch==rule::CR && !_multipartCtx.bdryPostCR )
         {
            _multipartCtx.bytesDone++;
            _multipartCtx.bdryPostCR = true;
            continue;
         }
         else if ( ch==rule::LF && _multipartCtx.bdryPostCR )
         {
            _multipartCtx.bytesDone++;

            _multipartCtx.bdryPostLF = true;  // First boundary found
            if (_multipartCtx.bdryBuff != _multipartCtx.boundary)
               return withError("Invalid part boundary", currentIdx);
            else
               continue;
         }
         else if ( ch != rule::CR && ch != rule::LF )
         {
            _multipartCtx.bdryBuff.append(1,ch);
            _multipartCtx.bytesDone++;
            continue;
         }
         else
         {
            // Invalid data in incoming buffer
            return withError("Invalid data for multipart", currentIdx);
         }
      } // End read part's boundary
   }

   return withSuccess(lastIndex, _multipartCtx.bytesDone);
}

} // namespace parser
} // namespace http
} // namespace tbs