#pragma once

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * CultureRecordVitek2Compact
 */
class CultureRecordVitek2Compact
   : public Record
{
protected:
   virtual void initFields();

public:
   CultureRecordVitek2Compact(const std::string& bciString);
   CultureRecordVitek2Compact() = default;
   ~CultureRecordVitek2Compact() = default;

   std::string labID() { return getFieldValue("ci"); }                            // ci
   std::string labIDSystemCode() { return getFieldValue("c0"); }                  // c0
   std::string cultureTypeCode() { return getFieldValue("ct"); }                  // ct
   std::string cultureTypeName() { return getFieldValue("cn"); }                  // cn

   void setLabID(const std::string& val) { setFieldValue("ci", val); }            // ci
   void setLabIDSystemCode(const std::string& val) { setFieldValue("c0", val); }  // c0
   void setCultureTypeCode(const std::string& val) { setFieldValue("ct", val); }  // ct
   void setCultureTypeName(const std::string& val) { setFieldValue("cn", val); }  // cn
};

} // namespace bci
} // namespace tbs 