#pragma once

#include <iostream>

namespace tbs {

class DataReader 
{
public:
   DataReader() = default;

   virtual ~DataReader() = default;

   // Read `bufSize` bytes from internal data into the buffer
   virtual std::streamsize read(uint8_t* buffer, std::streamsize bufSize) = 0;
   // Read `bufSize` bytes from internal data then write into ostream
   virtual std::streamsize read(std::ostream& ostream, std::streamsize bufSize) = 0;

   // Read `bufSize` bytes starting at a specific position `position` then copy to buffer
   virtual std::streamsize readAt(std::streampos position, uint8_t* buffer, std::streamsize bufSize) = 0;
   // Read `bufSize` bytes starting at a specific position `position`, the write into ostream
   virtual std::streamsize readAt(std::streampos position, std::ostream& ostream, std::streamsize bufSize) = 0;

   virtual std::streamsize dataSize() { return _dataSize; }
   virtual bool isOpen() = 0;

protected:
   std::streamsize _dataSize = 0;
};

} // namespace tbs