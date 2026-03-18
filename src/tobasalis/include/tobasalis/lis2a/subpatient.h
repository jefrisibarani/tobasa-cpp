#pragma once

#include "tobasalis/lis2a/subrecord.h"

namespace tbs {
namespace lis2a {

/** \ingroup LIS
 * PatientName
 * CLSI LIS02-A2 Patient Name - Record's component
 */
class PatientName
   : public SubRecord
{
public:
   PatientName();
   ~PatientName() = default;

   std::string lastName()   { return getFieldValue(1); }                   // 1
   std::string firstName()  { return getFieldValue(2); }                   // 2
   std::string middleName() { return getFieldValue(3); }                   // 3
   std::string suffix()     { return getFieldValue(4); }                   // 4
   std::string title()      { return getFieldValue(5); }                   // 5

   void setLastName(const std::string& val)   { setFieldValue(1, val); };  // 1
   void setFirstName(const std::string& val)  { setFieldValue(2, val); };  // 2
   void setMiddleName(const std::string& val) { setFieldValue(3, val); };  // 3
   void setSuffix(const std::string& val)     { setFieldValue(4, val); };  // 4
   void setTitle(const std::string& val)      { setFieldValue(5, val); };  // 5

protected:
   virtual void initFields();
};

} // namespace lis2a
} // namespace tbs 