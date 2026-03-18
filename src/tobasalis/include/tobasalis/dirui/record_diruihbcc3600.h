#pragma once

#include "tobasalis/lis/record.h"

namespace tbs {
namespace dirui {

/** \ingroup LIS
 * FieldsDirUiBcc3600
 */
class FieldsDirUiBcc3600
{
public:
   FieldsDirUiBcc3600()
   {
      index    = 0;
      name     = "";
      field    = "";
      value    = "";
      rawdata  = "";
   }

   FieldsDirUiBcc3600(
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
 * RecordDirUiBcc3600
 */
class RecordDirUiBcc3600
   : public lis::Record
{
public:
   RecordDirUiBcc3600(const std::string& lisString = "");
   ~RecordDirUiBcc3600();

   virtual bool fromString(const std::string& lisString = "");
   virtual std::string toString() { return ""; }
   virtual std::string recordTypeStr() { return "DirUiBcc3600"; }

   std::vector<FieldsDirUiBcc3600*>& getFields() { return _lisFields; }
   std::string date() const { return _date; }
   std::string no() const { return _no; }
   std::string id() const { return _id; }

protected:
   std::vector<FieldsDirUiBcc3600*> _lisFields;
   std::string _date, _no, _id;
};

} // namespace dirui
} // namespace tbs