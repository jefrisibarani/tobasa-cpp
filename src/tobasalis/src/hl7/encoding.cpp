#include <iostream>
#include <sstream>
#include <iomanip>
#include <tobasa/util_string.h>
#include "tobasalis/lis/common.h"
#include "tobasalis/hl7/exception.h"
#include "tobasalis/hl7/encoding.h"

namespace tbs {
namespace hl7 {

std::string HL7Encoding::allDelimiters()
{
   std::string val;
   val.append(1, _fieldDelimiter);
   val.append(1, _componentDelimiter);
   val.append(1, _repeatDelimiter);

   if (_escapeCharacter != (char)0)
      val.append(1, _escapeCharacter);

   val.append(1, _subComponentDelimiter);

   return val;
}

void HL7Encoding::evaluateDelimiters(const std::string& delimiters)
{
   _fieldDelimiter     = delimiters[0];
   _componentDelimiter = delimiters[1];
   _repeatDelimiter    = delimiters[2];

   if (delimiters[4] == _fieldDelimiter)
   {
      _escapeCharacter = (char)0;
      _subComponentDelimiter = delimiters[3];
   }
   else
   {
      _escapeCharacter = delimiters[3];
      _subComponentDelimiter = delimiters[4];
   }
}

void HL7Encoding::evaluateSegmentDelimiter(const std::string& message)
{
   //if (_instrumentType == lis::DEV_TEST_HL7)
   {
      //std::string delimiters[] = {"\r\n", "\n\r", "\n\n", "\r\r", "\r", "\n"};
      std::string delimiters[] = {"\r\n", "\n\r", "\r", "\n"};

      for (auto& delim: delimiters)
      {
         if (message.find(delim) != std::string::npos)
         {
            _segmentDelimiter = delim;
            return;
         }
      }

      throw HL7Exception("Segment delimiter not found in message", ERR_BAD_MESSAGE);
   }
}

std::string HL7Encoding::encode(const std::string& val)
{
   // Encoding methods based on https://github.com/elomagic/hl7inspector

   // check for present but null
   if (val == presentButNull() )
      return presentButNull();

   if (val.empty())
      return val;

   std::string sb;

   for (int i = 0; i < val.length(); i++)
   {
      char c = val[i];

      bool continueEncoding = true;
      if (c == '<')
      {
         continueEncoding = false;
         // special case <B>
         if (val.length() >= i + 3 && val[i + 1] == 'B' && val[i + 2] == '>')
         {
            sb.append(1, _escapeCharacter);
            sb.append(1, 'H');
            sb.append(1, _escapeCharacter);
            i += 2; // +1 in loop
         }
         // special case </B>
         else if (val.length() >= i + 4 && val[i + 1] == '/' && val[i + 2] == 'B' && val[i + 3] == '>')
         {
            sb.append(1, _escapeCharacter);
            sb.append(1, 'N');
            sb.append(1, _escapeCharacter);
            i += 3; // +1 in loop
         }
         // special case <BR>
         else if (val.length() >= i + 4 && val[i + 1] == 'B' && val[i + 2] == 'R' && val[i + 3] == '>')
         {
            sb.append(1, _escapeCharacter);
            sb.append(".br");
            sb.append(1, _escapeCharacter);
            i += 3; // +1 in loop
         }
         else
            continueEncoding = true;
      }

      if (continueEncoding)
      {
         if (c == _componentDelimiter)
         {
            sb.append(1, _escapeCharacter);
            sb.append("S");
            sb.append(1, _escapeCharacter);
         }
         else if (c == _escapeCharacter)
         {
            sb.append(1, _escapeCharacter);
            sb.append("E");
            sb.append(1, _escapeCharacter);
         }
         else if (c == _fieldDelimiter)
         {
            sb.append(1, _escapeCharacter);
            sb.append("F");
            sb.append(1, _escapeCharacter);
         }
         else if (c == _repeatDelimiter)
         {
            sb.append(1, _escapeCharacter);
            sb.append("R");
            sb.append(1, _escapeCharacter);
         }
         else if (c == _subComponentDelimiter)
         {
            sb.append(1, _escapeCharacter);
            sb.append("T");
            sb.append(1, _escapeCharacter);
         }
         else if (c == 10 || c == 13) // All other non-visible characters will be preserved
         {
            //string v = string.Format("{0:X2}", (int)c);
            std::stringstream ss;
            ss << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(c);
            std::string v = ss.str();            

            if ((v.length() % 2) != 0) // make number of digits even, this test would only be needed for values > 0xFF
               v = "0" + v;

            sb.append(1, _escapeCharacter);
            sb.append("X");
            sb.append(v);
            sb.append(1, _escapeCharacter);
         }
         else
         {
            sb.append(1, c);
         }
      }
   }

   return sb;
}

std::string HL7Encoding::decode(const std::string& encodedValue)
{
   if (encodedValue.empty())
      return encodedValue;

   std::string result;

   for (size_t i = 0; i < encodedValue.length(); i++)
   {
      char c = encodedValue[i];

      if (c != _escapeCharacter)
      {
         result.append(1, c);
         continue;
      }

      i++;

      int li = encodedValue.find(_escapeCharacter, i);

      if (li == std::string::npos)
      {
         // throw new HL7Exception("Invalid escape sequence in HL7 string");
         result.append(1, _escapeCharacter);
         result.append(1, encodedValue[i]);
         continue;
      }

      std::string seq = encodedValue.substr(i, li - i);
      i = li;

      if (seq.length() == 0)
         continue;

      if (seq == "H")         // Start higlighting
         result.append("<B>");
      else if (seq == "N")    // normal text (end highlighting)
         result.append("</B>");
      else if (seq == "F")    // field separator
         result.append(1, _fieldDelimiter);
      else if (seq == "S")    // component separator
         result.append(1, _componentDelimiter);
      else if (seq == "T")    // subcomponent separator
         result.append(1, _subComponentDelimiter);
      else if (seq == "R")    // repetition separator
         result.append(1, _repeatDelimiter);
      else if (seq == "E")    // escape character
         result.append(1, _escapeCharacter);
      else if (seq == ".br")
         result.append("<BR>");
      else
      {
         if (util::startsWith(seq, "X"))
         {
            // TODO_JEFRI: use function from tbs::crypt
            // Convert encoded hex string into string
            std::vector<unsigned char> bytes;
            for (int x = 0; x < seq.length() - 1; x++) 
            {
               if (x % 2 == 0) 
               {
                  std::string hexStr = seq.substr(x + 1, 2);
                  unsigned int byteValue;
                  std::stringstream ss;
                  ss << std::hex << hexStr;
                  ss >> byteValue;
                  bytes.push_back(static_cast<unsigned char>(byteValue));
               }
            }

            std::string byteAsStr(bytes.begin(), bytes.end());
            result.append(byteAsStr);
         }
         else
         {
            result.append(seq);
         }
      }
   }

   return result;
}


} // namespace hl7
} // namespace tbs
