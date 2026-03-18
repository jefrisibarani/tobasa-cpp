#pragma once

#include <fstream>
#include <string>
#include "tobasa/nonstd_span.hpp"
#include "tobasa/data_reader.h"

namespace tbs {

class BytesReader : public DataReader
{
public:
   explicit BytesReader(const nonstd::span<const unsigned char>& data);

   ~BytesReader();

   // Read `bufSize` bytes from internal data into the buffer
   std::streamsize read(uint8_t* buffer, std::streamsize bufSize);
   // Read `bufSize` bytes from internal data then write into ostream
   std::streamsize read(std::ostream& ostream, std::streamsize bufSize);

   // Read `bufSize` bytes starting at a specific position `position` then copy to buffer
   std::streamsize readAt(std::streampos position, uint8_t* buffer, std::streamsize bufSize);
   // Read `bufSize` bytes starting at a specific position `position`, the write into ostream
   std::streamsize readAt(std::streampos position, std::ostream& ostream, std::streamsize bufSize);

   bool isOpen();

private:
   nonstd::span<const unsigned char> _rawData;
   bool _opened = false;
};

} // namespace tbs