#pragma once

#include "tobasahttp/multipart.h"
#include "tobasahttp/http_parser.h"

namespace tbs {
namespace http {
namespace parser {

/** 
 * \brief Multipart body parsing context
 * \sa https://datatracker.ietf.org/doc/html/rfc1521#section-7.2.1
 */
struct MultipartContext
{
   // total bytes processed for all parts
   size_t         bytesDone       {0};

   // Multipart boundary as found in request header
   std::string    boundary        {};
   std::string    bdryToken       {};
   std::string    bdryTokenEnd    {};
   size_t         bdryTokenLen    {0};
   size_t         bdryTokenEndLen {0};

   // these values reset each time we found new part
   std::string    bdryBuff        {};
   bool           firstBdry       {true};
   bool           bdryPostCR      {false};
   bool           bdryPostLF      {false};
   bool           lastBdry        {false};

   std::string    body            {};
   HeadersContext headersCtx      {};
   bool           bodyCompleted   {false};
   
   // temporary folder to store uploaded file
   std::string    temporaryDir       {};
   
   // Tail buffer from last data chunk.
   // size is bdryTokenEndLen
   std::vector<uint8_t> tailBuffer  {};

   MultipartBody::PartPtr part    {nullptr};

   MultipartContext();
   
   void prepareForNextPart(const std::string& tempDir);
   void boundaryDone(bool done);
   bool applyBoundary(std::string_view rawBoundary);
   bool validateBoundary(std::string_view boundary);
   // is First Boundary found?
   bool boundaryDone();
   Headers headers();
   void initializePart();
   bool partInitialized() { return part!=nullptr && part->initialized; }
   void savePart(MultipartBodyUPtr &partHolder);
};

class MultipartParser
{
private:
   bool              _contentDone      {false};
   size_t            _contentLength    {0};

   MultipartContext  _multipartCtx     {};
   /// Initialized wwhen multipart body found request message
   MultipartBodyUPtr _multipartBody    {nullptr};

   // Multipart with Chunked Transfer Encoding
   bool              _chunkedMultipart  {false};
   
   std::string       _tmpDir           {};
   std::string       _id               {0}; // for debugging

public:
   MultipartParser(const std::string& temporaryDir="");
   ~MultipartParser();

   /// Get multipart body. Important: this will move the multipart body out of parser
   /// So make sure to call this only once after parsing done.
   /// Note: May return nullptr
   MultipartBodyUPtr multipartBody();

   /// Prepare/reset parser to read new HTTP multipart body
   void prepareForNextMessage();
   
   bool done() { return _contentDone; }
   bool applyBoundary(std::string_view rawBoundary);
   void contentLength(size_t length) { _contentLength = length; }
   Info parse(const uint8_t *data, size_t len);
   void chunkedMultipart(bool value) { _chunkedMultipart = value; }

   void temporaryDir(const std::string& path);

   void id(const std::string& id) { _id = id; }
   const std::string id() const   { return _id; }
};

} // namespace parser
} // namespace http
} // namespace tbs