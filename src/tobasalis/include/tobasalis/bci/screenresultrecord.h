#pragma once

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * ScreenResultRecord
 */
class ScreenResultRecord
   : public Record
{
protected:
   virtual void initFields();

public:
   ScreenResultRecord(const std::string& bciString);
   ScreenResultRecord() = default;
   ~ScreenResultRecord() = default;

   std::string resultSeparator() { return getFieldValue("ra"); }         // ra
   std::string finalOrganismCode() { return getFieldValue("o1"); }       // o1
   std::string finalOrganismName() { return getFieldValue("o2"); }       // o2
   std::string finalModifierCode() { return getFieldValue("o4"); }       // o4
   std::string finalModifierName() { return getFieldValue("o5"); }       // o5
   std::string organismGroupCode() { return getFieldValue("o6"); }       // o6
   std::string organismGroupName() { return getFieldValue("o7"); }       // o7
   std::string totalCountText() { return getFieldValue("o8"); }          // o8
   std::string totalCountCode() { return getFieldValue("o3"); }          // o3
   std::string hoursToPositive() { return getFieldValue("rh"); }         // rh

   void setResultSeparator(const std::string& val) { setFieldValue("ra", val); }     // ra
   void setFinalOrganismCode(const std::string& val) { setFieldValue("o1", val); }   // o1
   void setFinalOrganismName(const std::string& val) { setFieldValue("o2", val); }   // o2
   void setFinalModifierCode(const std::string& val) { setFieldValue("o4", val); }   // o4
   void setFinalModifierName(const std::string& val) { setFieldValue("o5", val); }   // o5
   void setOrganismGroupCode(const std::string& val) { setFieldValue("o6", val); }   // o6
   void setOrganismGroupName(const std::string& val) { setFieldValue("o7", val); }   // o7
   void setTotalCountText(const std::string& val) { setFieldValue("o8", val); }      // o8
   void setTotalCountCode(const std::string& val) { setFieldValue("o3", val); }      // o3
   void setHoursToPositive(const std::string& val) { setFieldValue("rh", val); }     // rh
};

} // namespace bci
} // namespace tbs 