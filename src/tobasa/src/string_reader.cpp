#include "tobasa/string_reader.h"

namespace tbs {

StringReader::StringReader(const std::string& data)
   : _stream(data)
{
   _opened = true;
   _dataSize = data.size();
}

StringReader::~StringReader()
{
   _opened = false;
}

// Read `bufSize` bytes from internal data into the buffer
std::streamsize StringReader::read(uint8_t* buffer,std::streamsize bufSize) 
{
   if (!_stream)
      return -1;

   _stream.read(reinterpret_cast<char*>(buffer), bufSize);

   // Return the actual number of bytes read
   return _stream.gcount();
}

// Read `bufSize` bytes from internal data then write into ostream
std::streamsize StringReader::read(std::ostream& ostream, std::streamsize bufSize)
{
   if (!_stream)
      return -1;

   char* buffer = new char[bufSize];

   _stream.read(buffer, bufSize);
   std::streamsize bytesRead = _stream.gcount();
   ostream.write(buffer, bytesRead);

   delete[] buffer;

   // Return the actual number of bytes read
   return bytesRead;
}

// Read `bufSize` bytes starting at a specific position `position` then copy to buffer
std::streamsize StringReader::readAt(std::streampos position, uint8_t* buffer, std::streamsize bufSize) 
{
   if (!_opened)
      return -1;

   // Move the stream pointer to the specified position
   _stream.seekg(position);

   if (!_stream)
      return -1; 

   _stream.read(reinterpret_cast<char*>(buffer), bufSize);

   // Return the actual number of bytes read
   return _stream.gcount();
}

// Read `bufSize` bytes starting at a specific position `position`, the write into ostream
std::streamsize StringReader::readAt(std::streampos position, std::ostream& ostream, std::streamsize bufSize)
{
   if (!_opened)
      return -1;

   // Move the stream pointer to the specified position
   _stream.seekg(position);

   if (!_stream)
      return -1;

   char* buffer = new char[bufSize];

   _stream.read(buffer, bufSize);
   std::streamsize bytesRead = _stream.gcount();
   ostream.write(buffer, bytesRead);

   delete[] buffer;

   // Return the actual number of bytes read
   return bytesRead;
}

bool StringReader::isOpen()
{
   return _opened;
}

} // namespace tbs