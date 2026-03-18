#pragma once

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * SpecimenRecordVitek2Compact
 */
class SpecimenRecordVitek2Compact
   : public Record
{
protected:
   virtual void initFields();

public:
   SpecimenRecordVitek2Compact(const std::string& bciString);
   SpecimenRecordVitek2Compact() = default;
   ~SpecimenRecordVitek2Compact() = default;

   std::string specimenSeparator() { return getFieldValue("si"); }                        // si
   std::string specimenSystemCode() { return getFieldValue("s0"); }                       // s0
   std::string specimenSourceCode() { return getFieldValue("ss"); }                       // ss
   std::string specimenSourceName() { return getFieldValue("s5"); }                       // s5
   std::string specimenCollectionDate() { return getFieldValue("s1"); }                   // s1
   std::string specimenCollectionTime() { return getFieldValue("s2"); }                   // s2
   std::string specimenReceiptDate() { return getFieldValue("s3"); }                      // s3
   std::string specimenReceiptTime() { return getFieldValue("s4"); }                      // s4
   std::string specimenCommentCode() { return getFieldValue("sc"); }                      // sc
   std::string specimenCommentText() { return getFieldValue("sn"); }                      // sn

   void setSpecimenSeparator(const std::string& val) { setFieldValue("si", val); }                // si
   void setSpecimenSystemCode(const std::string& val) { setFieldValue("s0", val); }               // s0
   void setSpecimenSourceCode(const std::string& val) { setFieldValue("ss", val); }               // ss
   void setSpecimenSourceName(const std::string& val) { setFieldValue("s5", val); }               // s5
   void setSpecimenCollectionDate(const std::string& val) { setFieldValue("s1", val); }           // s1
   void setSpecimenCollectionTime(const std::string& val) { setFieldValue("s2", val); }           // s2
   void setSpecimenReceiptDate(const std::string& val) { setFieldValue("s3", val); }              // s3
   void setSpecimenReceiptTime(const std::string& val) { setFieldValue("s4", val); }              // s4
   void setSpecimenCommentCode(const std::string& val) { setFieldValue("sc", val); }              // sc
   void setSpecimenCommentText(const std::string& val) { setFieldValue("sn", val); }              // sn
};

} // namespace bci
} // namespace tbs 