#include "tobasa/path.h"
#include "tobasa/file_reader.h"

namespace tbs {

FileReader::FileReader(const std::string& filePath)
   : file(filePath, std::ios::in | std::ios::binary)
{
   _opened = file.is_open();

   if (_opened)
      _dataSize = tbs::path::fileSize(filePath);
}

FileReader::~FileReader()
{
   if (file.is_open())
      file.close();
}

// Read `bufSize` bytes from internal data into the buffer
std::streamsize FileReader::read(uint8_t* buffer, std::streamsize bufSize) 
{
   if (!file)
      return -1;

   file.read(reinterpret_cast<char*>(buffer), bufSize);

   // Return the actual number of bytes read
   return file.gcount();
}

// Read `bufSize` bytes from internal data then write into ostream
std::streamsize FileReader::read(std::ostream& ostream, std::streamsize bufSize)
{
   if (!file)
      return -1;

   char* buffer = new char[bufSize];

   file.read(buffer, bufSize);
   std::streamsize bytesRead = file.gcount();
   ostream.write(buffer, bytesRead);

   delete[] buffer;

   // Return the actual number of bytes read
   return bytesRead;
}

// Read `bufSize` bytes starting at a specific position `position` then copy to buffer
std::streamsize FileReader::readAt(std::streampos position, uint8_t* buffer, std::streamsize bufSize) 
{
   if (!_opened)
      return -1;

   // Move the file pointer to the specified position
   file.seekg(position);

   if (!file)
      return -1;

   file.read(reinterpret_cast<char*>(buffer), bufSize);

   // Return the actual number of bytes read
   return file.gcount();
}

// Read `bufSize` bytes starting at a specific position `position`, the write into ostream
std::streamsize FileReader::readAt(std::streampos position, std::ostream& ostream, std::streamsize bufSize)
{
   if (!_opened)
      return -1;

   // Move the file pointer to the specified position
   file.seekg(position);

   if (!file)
      return -1;

   char* buffer = new char[bufSize];

   file.read(buffer, bufSize);
   std::streamsize bytesRead = file.gcount();
   ostream.write(buffer, bytesRead);

   delete[] buffer;

   // Return the actual number of bytes read
   return bytesRead;
}

bool FileReader::isOpen()
{
   return _opened;
}

} // namespace tbs