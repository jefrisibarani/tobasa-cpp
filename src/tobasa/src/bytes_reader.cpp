#include <cstring>  // for std::memcpy in linux with GCC 11.1
#include "tobasa/bytes_reader.h"

namespace tbs {

BytesReader::BytesReader(const nonstd::span<const unsigned char>& data)
   : _rawData(data)
{
   _opened = true;
   _dataSize = _rawData.size();
}

BytesReader::~BytesReader()
{
   _opened = false;
}

// Read `bufSize` bytes from internal data into the buffer
std::streamsize BytesReader::read(uint8_t* buffer, std::streamsize bufSize) 
{
   if ( _rawData.empty() || bufSize < 0 )
      return -1;
   // make sure bufSize non negative, so we can safely static_cast into size_t

   auto nSize = static_cast<std::size_t>(bufSize);
   std::size_t nBytes = _rawData.size() < nSize  ? _rawData.size() : nSize;
   std::memcpy(buffer, _rawData.data(), nBytes);
   return nBytes;
}

// Read `bufSize` bytes from internal data then write into ostream
std::streamsize BytesReader::read(std::ostream& ostream, std::streamsize bufSize)
{
   if ( _rawData.empty() || bufSize < 0 )
      return -1;
   // make sure bufSize non negative, so we can safely static_cast into size_t

   auto nSize = static_cast<std::size_t>(bufSize);
   std::size_t nBytes = _rawData.size() < nSize  ? _rawData.size() : nSize;
   ostream.write(reinterpret_cast<const char*>(_rawData.data()), nBytes);
   return nBytes;
}

// Read `bufSize` bytes starting at a specific position `position` then copy to buffer
std::streamsize BytesReader::readAt(std::streampos position, uint8_t* buffer, std::streamsize bufSize) 
{
   if (!_opened || bufSize < 0 || position < 0)
      return -1;
   // make sure bufSize & position non negative, so we can safely static_cast into size_t

   auto nSize = static_cast<size_t>(bufSize);
   auto pos   = static_cast<size_t>(position);

   if ( pos >= _rawData.size() )
      return -1; // Invalid position

   size_t available = _rawData.size() - pos;
   size_t nBytes    = available < nSize ? available : nSize;
   
   std::memcpy(buffer, _rawData.data() + pos, nBytes);

   return nBytes;
}

// Read `bufSize` bytes starting at a specific position `position`, the write into ostream
std::streamsize BytesReader::readAt(std::streampos position, std::ostream& ostream, std::streamsize bufSize)
{
   if (!_opened || bufSize < 0 || position < 0)
      return -1;
   // make sure bufSize & position non negative, so we can safely static_cast into size_t

   auto nSize = static_cast<size_t>(bufSize);
   auto pos   = static_cast<size_t>(position);

   if ( pos >= _rawData.size() )
      return -1; // Invalid position

   size_t available = _rawData.size() - pos;
   size_t nBytes    = available < nSize ? available : nSize;

   ostream.write( reinterpret_cast<const char*>(_rawData.data() + pos), nBytes );

   return nBytes;
}

bool BytesReader::isOpen()
{
   return _opened;
}

} // namespace tbs