#pragma once

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * GeneralRecord
 */
class GeneralRecord
   : public Record
{
protected:
   virtual void initFields();

public:
   GeneralRecord(const std::string& bciString);
   GeneralRecord() = default;
   ~GeneralRecord() = default;

   std::string messageType() { return getFieldValue("mt"); }
   std::string instrumentID() { return getFieldValue("id"); }
   std::string instrumentCode() { return getFieldValue("ii"); }
   std::string instrumentSN() { return getFieldValue("is"); }
   std::string testGroupCode() { return getFieldValue("it"); }

   void setMessageType(const std::string& val) { setFieldValue("mt", val); }
   void setInstrumentID(const std::string& val) { setFieldValue("id", val); }
   void setInstrumentCode(const std::string& val) { setFieldValue("ii", val); }
   void setInstrumentSN(const std::string& val) { setFieldValue("is", val); }
   void setTestGroupCode(const std::string& val) { setFieldValue("it", val); }
};

} // namespace bci
} // namespace tbs 