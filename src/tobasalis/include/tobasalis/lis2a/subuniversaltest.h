#pragma once

#include "tobasalis/lis2a/subrecord.h"

namespace tbs {
namespace lis2a {

/** \ingroup LIS
 * UniversalTestID
 * CLSI LIS02-A2 Universal Test ID - Record's component
 */
class UniversalTestID
   : public SubRecord
{
public:
   UniversalTestID();
   ~UniversalTestID() = default;

   std::string testID()           { return getFieldValue(1); }    // 1
   std::string testName()         { return getFieldValue(2); }    // 2
   std::string testType()         { return getFieldValue(3); }    // 3
   std::string manufacturerCode() { return getFieldValue(4); }    // 4
   std::string optionalField1()   { return getFieldValue(5); }    // 5
   std::string optionalField2()   { return getFieldValue(6); }    // 6
   std::string optionalField3()   { return getFieldValue(7); }    // 7
   std::string optionalField4()   { return getFieldValue(8); };   // 8

   void setTestID(const std::string& val)           { setFieldValue(1, val); }
   void setTestName(const std::string& val)         { setFieldValue(2, val); }
   void setTestType(const std::string& val)         { setFieldValue(3, val); }
   void setManufacturerCode(const std::string& val) { setFieldValue(4, val); }
   void setOptionalField1(const std::string& val)   { setFieldValue(5, val); }
   void setOptionalField2(const std::string& val)   { setFieldValue(6, val); }
   void setOptionalField3(const std::string& val)   { setFieldValue(7, val); }
   void setOptionalField4(const std::string& val)   { setFieldValue(8, val); }

protected:
   virtual void initFields();
};

} // namespace lis2a
} // namespace tbs 