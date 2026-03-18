#pragma once

#include "tobasalis/lis2a/record.h"

namespace tbs {
namespace lis2a {

/** \ingroup LIS
 * HeaderProcessingID
 */
enum class HeaderProcessingID
{
   Production     = 'P',
   Training       = 'T',
   Debugging      = 'D',
   QualityControl = 'Q',
   Unknown
};

/** \ingroup LIS
 * HeaderRecord
 * CLSI LIS02-A2 (formerly ASTM 1394-97) Message Header Record
 */
class HeaderRecord
   : public Record
{
protected:
   virtual void initFields();

public:
   /**
    * Construct Header Record.
    * @param lisString Raw Header record text
    * @param delimiter delimiter 
    * if lisString empty and delimiter not initialized, default delimiter values wiil be used
    *    field delimiter     = |
    *    repeat delimiter    = \
    *    component delimiter = !
    *    escape character    = ~
    *
    * if lisString contains valid Header record text, and delimiter not initialized,
    * delimiter from parsed lisString will be used. if delimiter initialized,
    * delimiter values will be used, ignoring delimiter from parsed text lisString
    */
   HeaderRecord(const std::string& lisString = "", lis::Delimiter delimiter = {} );
   ~HeaderRecord() = default;

   std::string recordTypeID()             { return getFieldValue(1); }     // 1
   std::string delimiterStr()             { return getFieldValue(2); }     // 2
   std::string messageControlID()         { return getFieldValue(3); }     // 3
   std::string accessPassword()           { return getFieldValue(4); }     // 4
   std::string senderID()                 { return getFieldValue(5); }     // 5
   std::string senderStreetAddress()      { return getFieldValue(6); }     // 6
   std::string reserved()                 { return getFieldValue(7); }     // 7
   std::string senderTelephoneNumber()    { return getFieldValue(8); }     // 8
   std::string characteristicsOfSender()  { return getFieldValue(9); }     // 9
   std::string receiverID()               { return getFieldValue(10); }    // 10
   std::string comment()                  { return getFieldValue(11); }    // 11
   std::string processingID()             { return getFieldValue(12); }    // 12 enum HeaderProcessingID
   HeaderProcessingID processingIDEnum();
   std::string version()                  { return getFieldValue(13); }    // 13
   /// LIS2-A, Section 6.6.2. YYYYMMDDHHMMSS
   std::string messageDateTime()          { return getFieldValue(14); }   // 14

   void setRecordTypeID(const std::string& val)            { setFieldValue(1, val); }
   void setDelimiterStr(const std::string& val)            { setFieldValue(2, val); }
   void setMessageControlID(const std::string& val)        { setFieldValue(3, val); }
   void setAccessPassword(const std::string& val)          { setFieldValue(4, val); }
   void setSenderID(const std::string& val)                { setFieldValue(5, val); }
   void setSenderStreetAddress(const std::string& val)     { setFieldValue(6, val); }
   void setReserved(const std::string& val)                { setFieldValue(7, val); }
   void setSenderTelephoneNumber(const std::string& val)   { setFieldValue(8, val); }
   void setCharacteristicsOfSender(const std::string& val) { setFieldValue(9, val); }
   void setReceiverID(const std::string& val)              { setFieldValue(10, val); }
   void setComment(const std::string& val)                 { setFieldValue(11, val); }
   void setProcessingID(HeaderProcessingID procId);
   void setProcessingID(const std::string& val)            { setFieldValue(12, val); }
   void setVersion(const std::string& val)                 { setFieldValue(13, val); }
   void setMessageDateTime(const std::string& val)         { setFieldValue(14, val); }
};

} // namespace lis2a
} // namespace tbs