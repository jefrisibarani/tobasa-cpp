#include <iostream>
#include "tobasahttp/multipart_body_reader.h"

namespace tbs {
namespace http {

MultipartBodyReader::MultipartBodyReader(ReadCallback readCb, span<const char> _buffer, size_t dataStart, size_t totalData)
   : _readCallback(std::move(readCb))
   , _buffer      { _buffer }
   , _dataStart   { dataStart }
   , _totalData   { totalData } {}

MultipartBodyReader::~MultipartBodyReader()
{
   std::cout << "TMPDBG ~MultipartBodyReader() \n";

   _dataHandler        = nullptr;
   _readCallback       = nullptr;
   _processBodyStarter = nullptr;
}


void MultipartBodyReader::setProcessBodyStarter(ProcessBodyStarter func) { _processBodyStarter = func;}
   
MultipartBodyReader::DataHandler MultipartBodyReader::chunkedHandler() 
{ 
   return [self=shared_from_this()](const uint8_t *data, size_t totalData)
   {
      return self->parseChunkedData(data,totalData);
   };
}

bool MultipartBodyReader::done() { return _done; }
void MultipartBodyReader::done(bool val) { _done=val;}
void MultipartBodyReader::setMultipartWithChunkedTransferEncoding() { _chunkedMultipart = true; }
bool MultipartBodyReader::chunkedMultipart() const { return _chunkedMultipart; }

/**
 * Read data from ServerConnection's read buffer.
 * DataHandler will be executed directly to parse initial data in read buffer.
 * Then if there are more data, ServerConnection will call feed() wich call will execute DataHandler
 */
void MultipartBodyReader::read(DataHandler dataHandler)
{
   _dataHandler = dataHandler;
   doRead();
}


void MultipartBodyReader::doRead()
{
   if ( !_firstReadDone && !_chunkedMultipart && _dataHandler && _readCallback )
   {
      // Read remaining data in ServerConnection's read buffer
      _firstReadDone = true;

      if (_totalData == 0)
         return _readCallback();

      auto info = _dataHandler(reinterpret_cast<const uint8_t *>(_buffer.data()), _totalData);
      if (info.success() )
      {
         if (!_done) // we need more data from Server Connection
            _readCallback();
      }
   }

   if ( _chunkedMultipart && _processBodyStarter && _dataHandler && _readCallback )
   {
      _firstReadDone = true;
      auto info = _processBodyStarter();
      if (info.success() )
      {
         if (!_done) // we need more data from Server Connection
            _readCallback();
      }
   }
}

/**
 * Called by ServerConnection to process incoming data
 */
parser::Info MultipartBodyReader::feed(span<const char> buffer, size_t totalData) 
{
   if (_dataHandler)
      return _dataHandler(reinterpret_cast<const uint8_t *>(buffer.data()), totalData);
   else
      return {false,"no handler", 0, {} ,0 };
}

parser::Info MultipartBodyReader::parseChunkedData(const uint8_t *data, size_t totalData)
{
   return _dataHandler(data, totalData);
}


} // namespace http
} // namespace tbs