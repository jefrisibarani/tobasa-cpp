#pragma once

#include "tobasalis/lis/parser.h"
#include "tobasalis/lis/delimiter.h"

namespace tbs {
namespace lis2a {

class Message;
class Record;

struct ParserOption
   : public lis::Delimiter
{
   int  resultRecordTotalField = 14;
   bool autoDetectDelimiter = false;

   lis::Delimiter delimiter()
   {
      lis::Delimiter delimiter;
      delimiter.fieldDelimiter     = fieldDelimiter;
      delimiter.repeatDelimiter    = repeatDelimiter;
      delimiter.componentDelimiter = componentDelimiter;
      delimiter.escapeCharacter    = escapeCharacter;
      
      return delimiter;
   }

   void delimiter(lis::Delimiter delimiter)
   {
      fieldDelimiter     = delimiter.fieldDelimiter;
      repeatDelimiter    = delimiter.repeatDelimiter;
      componentDelimiter = delimiter.componentDelimiter;
      escapeCharacter    = delimiter.escapeCharacter;
   }
};


/** \ingroup LIS
 * Parser
 * Parse raw clsi raw data and build complete clsi message
 */
class Parser
   : public lis::Parser
{
public:
   Parser(ParserOption option, bool enableLog=false);
   Parser() = default;
   ~Parser() = default;

   void parse(const std::string &data = "");
   void option(ParserOption option) { _option = option; }

private:
   ParserOption _option; 
   bool _enableLog = false; 
   std::shared_ptr<Message> _pLisMessage;

   /// Create a Record from one single line raw record
   /// then send the record to processRecord;
   void processLine(const std::string& data);

   /// Construct Message object from incoming Record
   /// if incoming record is HeaderRecord, contruct new Message object
   /// then send the message to SQL database
   void processRecord(std::shared_ptr<Record>& record);
};

} // namespace lis2a
} // namespace tbs 