#pragma once

#include "tobasalis/lis2a/record.h"

namespace tbs {
namespace lis2a {

/** \ingroup LIS
 * CommentRecord
 * CLSI LIS02-A2 Comment Record
 */
class CommentRecord
   : public Record
{
public:
   CommentRecord(const std::string& lisString = "");
   ~CommentRecord() = default;

   std::string recordTypeID() { return getFieldValue(1); }
   int sequenceNumber()       { return std::stoi(getFieldValue(2)); }
   std::string source()       { return getFieldValue(3); }
   std::string text()         { return getFieldValue(4); }
   std::string type()         { return getFieldValue(5); }

   void setRecordTypeID(const char id);
   void setSequenceNumber(int number)     { setFieldValue(2, std::to_string(number)); }
   void setSource(const std::string& val) { setFieldValue(3, val); }
   void setText(const std::string& val)   { setFieldValue(4, val); }
   void setType(const std::string& val)   { setFieldValue(5, val); }

protected:
   virtual void initFields();
};

} // namespace lis2a
} // namespace tbs