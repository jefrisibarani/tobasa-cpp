#pragma once

#include <memory>
#include <functional>
#include <string>
#include <tobasa/non_copyable.h>

namespace tbs {
namespace lis {

class Message;
class Record;

/** \ingroup LIS
 * Parser
 */
class Parser : private NonCopyable
{
public:
   virtual ~Parser() = default;
   virtual void parse(const std::string& data = "") = 0;

   /// Handler parsing error occured
   std::function<void(const std::string&)> onParserError;

   /// Handler when raw record string succesfully converted to Record object
   std::function<void(std::shared_ptr<Record>&)> onRecordReady;

   /// Handler when Message successfully constructed and ready for next processing
   std::function<void(std::shared_ptr<Message>&)> onMessageReady;

   std::string instrumentType() { return _instrumentType;}
   void instrumentType(const std::string&  value) { _instrumentType = value;} 

protected:
   std::string _instrumentType;   
};

} // namespace lis
} // namespace tbs 