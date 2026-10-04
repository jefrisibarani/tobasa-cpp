#pragma once

#include <memory>
#include <functional>
#include "tobasa/span.h"
#include "tobasahttp/http_parser.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/**
 * @class MultipartBodyReader
 * @brief Reads multipart/form-data request bodies as data arrives.
 *
 * This class parses multipart request bodies that can come in either a normal
 * Content-Length format or chunked transfer encoding. It works with the
 * server's connection loop: the server sends each new incoming buffer through
 * feed(), and this reader processes it with a handler.
 *
 * It keeps track of the parsing state across multiple network reads, and it can
 * ask the server to read more data when needed. This makes it useful for
 * handling uploads without waiting for the full body to arrive at once.
 *
 * ### Basic flow
 * 1. Create the reader with the first buffer and a callback to read more data.
 * 2. Call read() with a DataHandler to parse the incoming body.
 * 3. Feed each new network buffer into the reader.
 * 4. The handler returns parser::Info to say whether parsing succeeded and how
 *    much data was consumed.
 * 5. When the body is finished, call done(true).
 */
class MultipartBodyReader
   : public std::enable_shared_from_this<MultipartBodyReader>
{
public:
   /// Callback invoked when more data must be read from the network.
   using ReadCallback = std::function<void()>;

   /**
    * @brief Function that consumes incoming multipart body bytes.
    * 
    * @param data Pointer to raw data buffer.
    * @param totalData Number of bytes available.
    * @return parser::Info containing parsing success, error messages, etc.
    */
   using DataHandler = std::function<parser::Info(const uint8_t *data, size_t totalData)>;
   
   /// Starter function used when multipart body uses chunked transfer encoding.
   using ProcessBodyStarter = std::function<parser::Info()>;

private:
   DataHandler        _dataHandler        {nullptr};
   ReadCallback       _readCallback       {nullptr};
   ProcessBodyStarter _processBodyStarter {nullptr};

   // Initial buffer passed during construction (before async reads).
   span<const char> _buffer {};

   // Offset into _buffer where multipart data starts.
   size_t _dataStart {0};

   // Number of bytes of valid data in the initial buffer.
   size_t _totalData {0};

   // Indicates whether parsing is complete.
   bool _done {false};

   bool _firstReadDone {false};

   // True if the multipart body uses Transfer-Encoding: chunked.
   bool _chunkedMultipart {false};

public:

   /**
    * @brief Construct a MultipartBodyReader.
    *
    * @param readCb Callback to request more data from the network.
    * @param buffer Initial buffer containing request body data.
    * @param dataStart Offset where multipart data begins in buffer.
    * @param totalData Number of valid bytes in buffer.
    */
   MultipartBodyReader(ReadCallback readCb, span<const char> _buffer, size_t dataStart, size_t totalData);

   ~MultipartBodyReader();

   /**
    * @brief Mark reader as complete or query its completion state.
    */
   bool done();
   void done(bool val);

   /// Flag this reader as handling multipart with chunked transfer encoding.
   void setMultipartWithChunkedTransferEncoding();
   
   /// Query if this reader is handling multipart with chunked transfer encoding.
   bool chunkedMultipart() const;

   /// Set the function to be used when starting chunked multipart parsing.
   void setProcessBodyStarter(ProcessBodyStarter func);

   /**
    * @brief Handler for multipart with chunked transfer encoding.
    *
    * Returns a DataHandler function object that can be passed into read()
    * when multipart bodies are transmitted with Transfer-Encoding: chunked.
    *
    * @return DataHandler that decodes chunked multipart data.
    */
   DataHandler chunkedHandler() ;

   /**
    * @brief Start reading and parsing multipart data.
    *
    * Installs a DataHandler which is called immediately with any data already
    * present in the initial buffer. If more data is required, the ReadCallback
    * is invoked to schedule additional network reads.
    *
    * @param dataHandler Function to handle incoming multipart body bytes.
    */
   void read(DataHandler dataHandler);


   /**
    * @brief Deliver new incoming data from the server connection.
    *
    * This should be called by the connection loop (inside ServerConnection) whenever new data arrives
    * from the client. The installed DataHandler is executed to parse the bytes.
    *
    * @param buffer     Buffer of raw HTTP body data.
    * @param totalData  Number of bytes available in buffer.
    * @return parser::Info Parsing result (success flag, bytes consumed, message).
    */
   parser::Info feed(span<const char> buffer, size_t totalData);


private:
   
   void doRead();
   parser::Info parseChunkedData(const uint8_t *data, size_t totalData);
};

 /** @}*/

} // namespace http
} // namespace tbs