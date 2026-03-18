#pragma once

#include <memory>
#include <string>

namespace tbs {
namespace hl7 {

class HL7Encoding
{
private:
   char _fieldDelimiter          = '|';
   char _componentDelimiter      = '^';
   char _repeatDelimiter         = '~';
   char _escapeCharacter         = '\\';
   char _subComponentDelimiter   = '&';
   std::string _segmentDelimiter = "\r";
   std::string _presentButNull   = "\"\"";

   std::string _instrumentType;

public:

   HL7Encoding() = default;
   ~HL7Encoding() = default;

   char fieldDelimiter()          { return _fieldDelimiter; }
   char componentDelimiter()      { return _componentDelimiter; }
   char repeatDelimiter()         { return _repeatDelimiter; }
   char escapeCharacter()         { return _escapeCharacter; }
   char subComponentDelimiter()   { return _subComponentDelimiter; }
   std::string segmentDelimiter() { return _segmentDelimiter; }
   std::string presentButNull()   { return _presentButNull; }

   void fieldDelimiter(char c)          { _fieldDelimiter = c; }
   void componentDelimiter(char c)      { _componentDelimiter = c; }
   void repeatDelimiter(char c)         { _repeatDelimiter = c; }
   void escapeCharacter(char c)         { _escapeCharacter = c; }
   void subComponentDelimiter(char c)   { _subComponentDelimiter = c; }
   void segmentDelimiter(const std::string& val) { _segmentDelimiter = val; }
   void presentButNull(const std::string& val)   { _presentButNull = val; }


   std::string allDelimiters();

   void evaluateDelimiters(const std::string& delimiters);

   void evaluateSegmentDelimiter(const std::string& message);

   std::string encode(const std::string& val);

   std::string decode(const std::string& encodedValue);

   std::string instrumentType() { return _instrumentType;}
   void instrumentType(const std::string&  value) { _instrumentType = value;}
};

using HL7EncodingPtr = std::shared_ptr<HL7Encoding>;

} // namespace hl7
} // namespace tbs
