#pragma once

#include "tobasalis/lis/record.h"

namespace tbs {
namespace dirui {

/** \ingroup LIS
 * DirUih500Fields
 */
class DirUih500Fields
{
public:

   DirUih500Fields()
   {
      index    = 0;
      name     = "";
      field    = "";
      value    = "";
      rawdata  = "";
   }

   DirUih500Fields(
      int                _idx,
      const std::string& _name,
      const std::string& _field,
      const std::string& _value,
      const std::string& _rawdata)
   {
      index    = _idx;
      name     = _name;
      field    = _field;
      value    = _value;
      rawdata  = _rawdata;
   }

   int         index;
   std::string name;
   std::string field;
   std::string value;
   std::string rawdata;
};


/** \ingroup LIS
 * DirUih500Record
 */
class DirUih500Record 
   : public lis::Record
{
public:
   DirUih500Record(const std::string& lisString="");
   ~DirUih500Record();

   virtual bool fromString(const std::string& lisString = "");
   virtual std::string toString() { return ""; }
   virtual std::string recordTypeStr() { return "DirUiH500"; }

   std::vector<DirUih500Fields*>& getFields() { return _lisFields; }
   std::string date() const { return _date; }
   std::string no() const { return _no; }
   std::string id() const { return _id; }

protected:
   std::vector<DirUih500Fields*> _lisFields;
   std::string _date, _no, _id;
};

} // namespace dirui
} // namespace tbs