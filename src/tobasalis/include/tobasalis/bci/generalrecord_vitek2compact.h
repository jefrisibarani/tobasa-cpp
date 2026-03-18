#pragma once

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * GeneralRecordVitek2Compact
 */
class GeneralRecordVitek2Compact
   : public Record
{
protected:
   virtual void initFields();

public:
   GeneralRecordVitek2Compact(const std::string& bciString);
   GeneralRecordVitek2Compact() = default;
   ~GeneralRecordVitek2Compact() = default;

   std::string messageType() { return getFieldValue("mt"); }
   std::string instrumentCode() { return getFieldValue("ii"); }
   std::string instrumentSN() { return getFieldValue("is"); }
   std::string testGroupCode() { return getFieldValue("it"); }

   void setMessageType(const std::string& val) { setFieldValue("mt", val); }
   void setInstrumentCode(const std::string& val) { setFieldValue("ii", val); }
   void setInstrumentSN(const std::string& val) { setFieldValue("is", val); }
   void setTestGroupCode(const std::string& val) { setFieldValue("it", val); }
};

} // namespace bci
} // namespace tbs 