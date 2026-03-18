#pragma once

#include "tobasalis/lis/record.h"

namespace tbs {
namespace lis {

/** \defgroup LIS Laboratorium Information System
 */

struct MessageOption
{
   bool        autoDetectDelimiter = false;
   std::string fieldDelimiter;
   std::string repeatDelimiter;
   std::string componentDelimiter;
   std::string escapeCharacter;
   std::string subComponentDelimiter;
};


/** \ingroup LIS
 * Message
 * 
 * Valid Vendor protocol ID
 *    lis::MSG_LIS2A
 *    lis::MSG_HL7
 */
class Message
{
public:
   Message();
   virtual ~Message();

   void setHeader(const RecordPtr& header) { _pHeader = header; }
   RecordPtr getHeader() const { return _pHeader; }
   RecordPtr getLastTouchedRecord() { return _pLastTouchedRecord; }
   void setLastTouchedRecord(const RecordPtr& record) { _pLastTouchedRecord = record; }

   virtual std::string toString() { return "";};

   void setOption(MessageOption _option) {_option = _option; }
   MessageOption getOption() { return _option; }

   std::string vendorProtocolId() { return _vendorProtocolId;}
   void vendorProtocolId(const std::string&  id) { _vendorProtocolId = id;}

   std::string instrumentType() { return _instrumentType;}
   void instrumentType(const std::string&  value) { _instrumentType = value;}
   

   std::string internalId() const;
   static int instanceCreated();

private:
   std::string    _internalId;

protected:
   RecordPtr      _pHeader;
   RecordPtr      _pLastTouchedRecord;
   MessageOption  _option;
   std::string    _vendorProtocolId;
   std::string    _instrumentType;
};

using MessagePtr = std::shared_ptr<lis::Message>;

} // namespace lis
} // namespace tbs 