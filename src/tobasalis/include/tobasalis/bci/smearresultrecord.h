#pragma once

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * SmearResultRecord
 */
class SmearResultRecord
   : public Record
{
protected:
   virtual void initFields();

public:
   SmearResultRecord(const std::string& bciString);
   SmearResultRecord() = default;
   ~SmearResultRecord() = default;

   std::string resultSeparator() { return getFieldValue("ra"); }         // ra
   std::string smearCode() { return getFieldValue("rs"); }               // rs
   std::string smearName() { return getFieldValue("rm"); }               // rm
   std::string finalModifierCode() { return getFieldValue("o4"); }       // o4
   std::string finalModifierName() { return getFieldValue("o5"); }       // o5

   void setResultSeparator(const std::string& val) { setFieldValue("ra", val); }   // ra
   void setSmearCode(const std::string& val) { setFieldValue("rs", val); }         // rs
   void setSmearName(const std::string& val) { setFieldValue("rm", val); }         // rm
   void setFinalModifierCode(const std::string& val) { setFieldValue("o4", val); } // o4
   void setFinalModifierName(const std::string& val) { setFieldValue("o5", val); } // o5
};

} // namespace bci
} // namespace tbs 