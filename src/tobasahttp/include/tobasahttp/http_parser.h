#pragma once

#include <functional>
#include <tobasa/non_copyable.h>
#include "tobasahttp/status_codes.h"
#include "tobasahttp/headers.h"

namespace tbs {
namespace http {

   class MultipartBody;

namespace parser {

/** \addtogroup HTTP
 * @{
 */

/** 
 * \brief Parsing operation mode.
 * REQUEST:  Parse HTTP Request
 * RESPONSE: Parse HTTP Response
 */
enum class Type
{
   REQUEST,
   RESPONSE
};


/** 
 * \brief Parser's parse() method return value.
 */
class Info
{
private:

   bool             _success    {true};
   std::string_view _message    {};
   size_t           _lastIndex  {0};
   HttpStatus       _httpStatus {};
   size_t           _bytesRead  {0};

public:

   Info(bool success, std::string_view message, size_t lastIndex, HttpStatus httpStatus, size_t bytesRead)
      : _success    { success }
      , _message    { message }
      , _lastIndex  { lastIndex}
      , _httpStatus {httpStatus }
      , _bytesRead  { bytesRead} {}
    
   Info() {}

   void success(bool success)             { _success    = success; }
   void message(std::string_view message) { _message    = message; }
   void lastIndex(size_t lastIndex)       { _lastIndex  = lastIndex; }
   void httpStatus(HttpStatus httpStatus) { _httpStatus = httpStatus; }
   void bytesRead(size_t nread)           { _bytesRead  = nread; }

   bool success()             { return _success; }
   std::string_view message() { return _message; }
   size_t lastIndex()         { return _lastIndex; }
   HttpStatus httpStatus()    { return _httpStatus; }
   size_t bytesRead()         { return _bytesRead; }
};


/** 
 * \brief HTTP message start-line or header.
 * \details
 * https://datatracker.ietf.org/doc/html/rfc7230#section-3.1 (start-line)
 * request-line   = method SP request-target SP HTTP-version CRLF
 * status-line    = HTTP-version SP status-code SP reason-phrase CRLF
 * 
 * https://datatracker.ietf.org/doc/html/rfc7230#section-3.2 (header field)
 * header-field   = field-name ":" OWS field-value OWS
 * field-name     = token
 * field-value    = *( field-content / obs-fold )
 * field-content  = field-vchar [ 1*( SP / HTAB ) field-vchar ]
 * field-vchar    = VCHAR / obs-text
 * obs-fold       = CRLF 1*( SP / HTAB )
 *                   ; obsolete line folding
 * 
 * https://datatracker.ietf.org/doc/html/rfc7230#section-3.2.4
 * The contents within a given field
 * value are not parsed until a later stage of message interpretation
 * (usually after the message's entire header section has been
 * processed)
 */

 struct Line
{
   std::string fieldName  {};
   std::string fieldValue {};
   bool valid();
};


struct StartLineContext
{
   std::string data      {};
   bool        gotCR     {false};
   bool        gotLF     {false};
   char        prevChar  {'\0'};
   bool done();
   bool valid();
   bool consume(char ch);
};


/** 
 * \brief HTTP headers parsing context.
 * This is working context for parsing HTTP headers.
 */
struct HeadersContext
{
   struct LineContext
   {
      std::string nameBuff  {};
      std::string valueBuff {};
      bool        gotCR     {false};
      bool        gotLF     {false};
      bool        gotCOLON  {false};
      char        prevChar  {'\0'};

      bool done();
      bool valid();
      bool consume(char ch);
   };

   std::vector<Line> lines         {};
   bool              gotCR         {false};
   bool              gotLF         {false};
   size_t            length        {0};
   LineContext       lineCtx       {};
   size_t            totalLine     {0};
   size_t            consumedByte  {0};
   size_t            maxHeaderSize {8*1024}; // default 8KB

   void startNewLine();
   bool done();
   void saveLine();
   Line findLine(const std::string& name);
   bool lineExists(const std::string& name, const std::string& value);
};


/** 
 * \brief Chuncked data parsing context.
 */
struct ChunkedContext
{
   bool           sizeCR     {false};
   bool           sizeLF     {false};
   bool           dataCR     {false};
   bool           dataLF     {false};
   bool           lastCR     {false};
   bool           lastLF     {false};
   bool           finalChunk {false};
   uint64_t       dataCount  {0};
   std::string    rawSize    {};      // hexadecimal (no 0x prefix)
   
   bool           hasTrailer {false};
   HeadersContext headersCtx {};

   bool dataDone()  { return dataCR && dataLF; }
   bool sizeDone()  { return sizeCR && sizeLF; }

   uint64_t dataSize()
   {
      return std::stoull(rawSize, 0, 16);
   }

   /// Prepare context for next chunked data
   void prepareForNextChunk();
};

class MultipartParser;

/** 
 * \brief Http Message Parser
 * Parse Request, Response, Chunked body, Multipart body
 */
class Parser : private NonCopyable
{
public:

   /** 
    * \brief Callback called when parser found http request method.
    * If http method forbidden/not implemented,
    * implementation must return false and correctly
    * set relevant http status code.
    * Parser will stop parsing if callback returned false;
    */
   std::function<bool(const std::string method, HttpStatus& httpStatus)> onValidateRequestMethod { nullptr };

   /// Validate Headers callback
   std::function<bool(Parser& parser, HttpStatus& httpStatus)> onValidateHeaders { nullptr };

   /// Validate Headers callback
   std::function<void(const std::string& eventType)> onEventDone { nullptr };

   /// Callback called when processing multipart with chunked transfer encoding
   std::function<Info(const uint8_t *data, size_t totalData)> parseChunkedMultipartHandler { nullptr };
   
   /// Callback to initiate a call to processBody()
   /// This callback will only be executed once by MultipartBodyReader
   std::function<Info()> processBodyStarter { nullptr };

private:

   // Parsing mode(REQUEST/RESPONSE)
   Type                 _type               {Type::REQUEST};
   std::string          _typeString         {};
   uint32_t             _parsingId          {0};
   std::string          _temporaryDir       {};
   uint64_t             _connId             {0}; // for debugging

   /// Parsing stuff
   /// \{
   /// Reference to buffer(client request) to be parsed. This buffer owned by Connection
   std::vector<char>&   _readBuffer;
   bool                 _isReading           {false};
   bool                 _contentDone         {false};
   bool                 _headersDone         {false};
   bool                 _hasMultipart        {false};
   bool                 _hasChunkedEncoding  {false};
   bool                 _hasContentLength    {false};
   size_t               _totalParsedBytes    {0};

   StartLineContext     _startLineContext    {};
   ChunkedContext       _chunkedCtx          {};

   /// Parsed headers holder
   HeadersContext       _headersCtx          {};
   /// \}

   /// Request/Response properties
   /// \{
   uint16_t             _majorVersion        {1};
   uint16_t             _minorVersion        {1};
   /// The body of request/response
   std::string          _content             {};
   size_t               _contentLength       {0};
   /// \}

   /// Response properties
   /// \{
   uint16_t             _statusCode          {0};
   std::string          _statusMessage       {};
   /// \}

   /// Parsed request properties
   /// \{
   std::string          _method              {};
   // https://datatracker.ietf.org/doc/html/rfc7230#section-5.3
   //  request-target = origin-form
   //                 / absolute-form
   //                 / authority-form
   //                 / asterisk-form
   std::string          _requestTarget       {};
   std::string          _requestLine         {};
   /// \}

   /// The maximum length of a request header
   size_t               _maxHeaderSize       { 8 * 1024 };
   
   /// Set true to parse/process multipart body.
   bool                 _enableMultipart     { true };
   
   std::unique_ptr<MultipartParser> _multipartParser {nullptr};
   
   std::unique_ptr<MultipartBody> _multipartBody   {nullptr};
   
   bool                 _100ContinueSent     {false};

public:

   /** Contruct Parser
    * @param parserType :  Parser type
    * @param buffer     :  Reference to incoming char buffer data.
    */
   explicit Parser(Type parserType, std::vector<char>& buffer, 
      size_t maxHeaderSize=8*1024, bool enableMultipart=true, const std::string& temporaryDir={});

   ~Parser();
   
   void parsingId(uint32_t id) { _parsingId = id; }
   const uint32_t parsingId() const   { return _parsingId; }

   void connId(uint64_t id) { _connId = id; }
   const uint64_t connId() const   { return _connId; }

   void type(Type parserType);
   bool done() const;
   bool hasContent() const;
   bool headersDone() const;
   bool contentDone() const;
   std::string method() const;
   std::string requestTarget() const;
   std::string requestLine() const;
   uint16_t majorVersion() const;
   uint16_t minorVersion() const;

   /// Get parsed headers. Important: this will move the headers out of parser.
   /// So make sure to call this only once after parsing done.
   Headers headers() const;

   /// Get parsed content/body. Important: this will move the content out of parser
   /// So make sure to call this only once after parsing done.
   std::string content();

   /// Get multipart body. Important: this will move the multipart body out of parser
   /// So make sure to call this only once after parsing done.
   /// Note: May return nullptr
   std::unique_ptr<MultipartBody> multipartBody();

   bool isReading() const;

   void isReading(bool value);

   HeadersContext& headerContext();

   /**
    * Find parsed header by name (case-insensitive).
    * This will return the first matched header.
    * Important: To get valid header line, make sure parsing headers is done
    * and headers() is not called yet.
    */
   Field findHeader(const std::string& name);

   /** 
    * Parse received data.
    * This function runs every time async_read_some(in HttpConnection)
    * got some data, until all data received
    */
   Info parse(size_t bytesTransferred);

   uint16_t statusCode() const;
   std::string statusMessage() const;

   /** 
    * \brief Prepare/reset parser to read new http mesage
    */
   void prepareForNextMessage();

   size_t totalParsedBytes()  { return _totalParsedBytes; }
   bool hasContentLength()    { return _hasContentLength; }
   bool hasChunkedEncoding()  { return _hasChunkedEncoding; }
   bool hasMultipart()        { return _hasMultipart; }
   size_t contentLength()     { return _contentLength; }
   void markContinueSent()    { _100ContinueSent = true; }

private:

   struct BufferInfo
   {
      size_t dataStart;
      size_t totalData;
   };
   /// Compute the real start of body and total data need to be read from read buffer
   /// @note bodyStartPosition id not current read buffer index
   BufferInfo ComputeBufferInfo(size_t bytesTransferred, size_t bodyStartPosition);

   /// Parse incoming http message with own parser
   Info parseWithOwnParser(size_t bytesTransferred);

   /// Parse incoming http response start line strict syntax checking
   Info parseResponseStartLine(std::string_view rawtext);

   /// Parse incoming http request start line with strict syntax checking
   Info parseRequestStartLine(std::string_view rawtext);

   /// Parse and Collect all headers
   Info retrieveHeaders(HeadersContext& hdrCtx, size_t dataStart, size_t totalData);

   /// Process completed headers, then if request has body, call processBody()
   Info processHeader(size_t bytesTransferred, size_t lastIndex);

   /**
    * \brief 
    *  Check wheter to continue parsing multipart body with this parser instance 
    *  or forward/stream to multipart parser instance inside a http request handler (maybe a middleware)
    *  \note bodyStartPosition id not current read buffer index
    *
    *  When this parser instance set to parse multipart & multipart with chunked transfer
    *  just call processBody() directly.
    *  If not, there are two ways:
    *  1. Multipart with Chunked Transfer Encoding.
    *     For the first time only,
    *     * Initialize processBodyStarter.
    *     * Immediately return back control to ServerConnection to process/retrieve data,
    *       which will call readBody(), and call _parser.parse().
    *     Next call to checkNeedProcessBody() will directly call processBody()
    *  
    *  2. Normal Multipart.
    *     Never call processBody().
    *     Control returned back to ServerConnection, which will call readBody(), 
    *     and let MultipartBodyReader's onData to do the job.
    */
   Info checkNeedProcessBody(size_t bytesTransferred, size_t bodyStartPosition);

   /// Retrieve request body
   Info processBody(size_t bytesTransferred, size_t bodyStartPosition);
   Info retrieveMultipartBody(size_t dataStart, size_t totalData);
   Info retrieveBody(size_t dataStart, size_t totalData);
   Info retrieveChunkedBody(size_t dataStart, size_t totalData);
};

/// Helper function for returning parsing status code
Info withSuccess(size_t lastIndex=0, size_t bytesRead=0);
Info withError(std::string_view message, size_t lastIndex=0, HttpStatus httpStatus = HttpStatus{StatusCode::BAD_REQUEST}, size_t bytesRead=0 );

Info parseHeaders(HeadersContext& hdrCtx, const uint8_t *data, size_t dataStart, size_t totalData);

/** @}*/

} // namespace parser
} // namespace http
} // namespace tbs