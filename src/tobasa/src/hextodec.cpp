#include <cassert>
#include <stdexcept>
#include "tobasa/hextodec.h"

namespace tbs {
namespace crypt {

/// Array used in DecToHex conversion routine.
static const char hexArray[] = "0123456789ABCDEF";

int hexToDec(const char hex)
{
   if (hex >= 'A' && hex <= 'F') return hex - 'A' + 10;
   if (hex >= 'a' && hex <= 'f') return hex - 'a' + 10;
   if (hex >= '0' && hex <= '9') return hex - '0';
   
   return -1;
}

int hexToDec(const char* buf)
{
   int firstDigit, secondDigit;

   firstDigit = hexToDec(buf[0]);

   if (! (firstDigit >= 0 && firstDigit <= 15) )
      throw std::runtime_error("Invalid argument for hexToDec");

   secondDigit = hexToDec(buf[1]);

   if (! (secondDigit >= 0 && secondDigit <= 15) )
      throw std::runtime_error("Invalid argument for hexToDec");

   return firstDigit * 16 + secondDigit;
}

int hexToDec(const std::string& str)
{
   if (! (str.length() >= 2) )
      throw std::runtime_error("Invalid argument for hexToDec");

   char buf[2];
   buf[0] = str.at(0);
   buf[1] = str.at(1);

   return hexToDec(buf);
}

void decToHex(byte_t dec, char* buf)
{
   if (! buf )
      throw std::runtime_error("Invalid argument for decToHex");

   buf[0] = hexArray[dec >> 4];
   buf[1] = hexArray[dec & 0x0F];
   buf[2] = 0;
}

void decToHex(byte_t dec, char* ch1, char* ch2)
{
   if (! (ch1 && ch2) )
      throw std::runtime_error("Invalid argument for decToHex");

   *ch1 = hexArray[dec >> 4];
   *ch2 = hexArray[dec & 0x0F];
}

std::string decToHex(byte_t dec)
{
   char buf[3];
   decToHex(dec, buf);
   return std::string(buf);
}

void hexDecode(const std::string& hexString, byte_t* outBuffer)
{
   size_t strLenOri  = hexString.size();

   if (strLenOri % 2)
      throw std::runtime_error("invalid hexadecimal input string");

   for (size_t i = 0; i < strLenOri; i = i + 2)
   {
      char buf[2];
      buf[0] = hexString.at(i);
      buf[1] = hexString.at(i+1);

      byte_t val = (byte_t) hexToDec(buf);
      *outBuffer = val;
      outBuffer++;
   }
}

std::string hexEncode(byte_t buffer[], size_t bufferLength)
{
   if (! buffer )
      throw std::runtime_error("Invalid argument for hexEncode");

   std::string outStr;
   for (size_t i = 0; i < bufferLength; i++)
   {
      char hex[3];
      hex[0] = hexArray[buffer[i] >> 4];
      hex[1] = hexArray[buffer[i] & 0x0F];
      hex[2] = 0;

      outStr.append(hex);
   }

   return outStr;
}

void hexEncode(byte_t buffer[], size_t bufferLength, std::string& output)
{
   if (! buffer )
      throw std::runtime_error("Invalid argument for hexEncode");

   for (size_t i = 0; i < bufferLength; i++)
   {
      char hex[3];
      hex[0] = hexArray[buffer[i] >> 4];
      hex[1] = hexArray[buffer[i] & 0x0F];
      hex[2] = 0;

      output.append(hex);
   }
}

} // namespace crypt
} // namespace tbs