#pragma once

#include "tobasalis/lis2a/subrecord.h"

namespace tbs {
namespace lis2a {

/** \ingroup LIS
 * StartingRange
 * CLSI LIS02-A2 Starting Range  - Record's component
 */
class StartingRange
   : public SubRecord
{
public:
   StartingRange();
   ~StartingRange() = default;

   std::string patientID()  { return getFieldValue(1); }           // 1
   std::string specimenID() { return getFieldValue(2); }           // 2
   std::string reserved()   { return getFieldValue(3); }           // 3

   void setPatientID(const std::string& val)  { setFieldValue(1, val); }
   void setSpecimenID(const std::string& val) { setFieldValue(2, val); }
   void setReserved(const std::string& val)   { setFieldValue(3, val); }

protected:
   virtual void initFields();
};

} // namespace lis2a
} // namespace tbs 