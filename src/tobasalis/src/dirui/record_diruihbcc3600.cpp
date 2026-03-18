#include <string>
#include <tobasa/util_string.h>
#include "tobasalis/dirui/record_diruihbcc3600.h"

namespace tbs {
namespace dirui {

RecordDirUiBcc3600::RecordDirUiBcc3600(const std::string& lisString)
   : lis::Record(lisString) {}

RecordDirUiBcc3600::~RecordDirUiBcc3600()
{
   _lisFields.clear();
}

bool RecordDirUiBcc3600::fromString(const std::string& lisString)
{
   if (lisString.empty() && _lisString.empty())
      return false;

   if (_lisString.empty())
      _lisString = lisString;

   // split incoming frame data into records by using CRLF as separator
   std::vector<std::string> array = tbs::util::split(_lisString, "\r\n");

   // _date, _no, _id;  This record/result global values
   size_t i = 0;
   for (i = 0; i < array.size() - 1; i++)
   {
      std::string line = array.at(i);
      if (!line.empty())
      {
         std::string name_, field_, value_, rawdata_;
         // -------------------------------------------------------
         // Extract  Field name
         if (line.find("ID:") != std::string::npos || line.find("No.") != std::string::npos)
            name_ = line.substr(0, 3);

         if (line.find("Color") != std::string::npos || line.find("Clarity") != std::string::npos)
            name_ = line.substr(0, 7);
         else
            name_ = line.substr(0, 5);


         tbs::util::trim(name_);

         // Ectract Field value
         if (line.find("ID:") != std::string::npos)
            value_ = line.substr(4, line.length());
         else if (line.find("Date:") != std::string::npos)
            value_ = line.substr(6, line.length());
         else if (line.find("Color") != std::string::npos || line.find("Clarity") != std::string::npos)
            value_ = line.substr(8, line.length());
         else
            value_ = line.substr(5, line.length());


         rawdata_ = line;
         // -------------------------------------------------------
         if (name_.find("Date:") != std::string::npos)
         {
            field_ = "date";
            _date = value_;
         }
         else if (name_.find("No.") != std::string::npos)
         {
            field_ = "nomor";
            _no = value_;
         }
         else if (name_.find("ID:") != std::string::npos)
         {
            field_ = "id";
            _id = value_;
         }
         else if (name_.find("UBG") != std::string::npos)
            field_ = "ubg";
         else if (name_.find("BIL") != std::string::npos)
            field_ = "bil";
         else if (name_.find("KET") != std::string::npos)
            field_ = "ket";
         else if (name_.find("BLD") != std::string::npos)
            field_ = "bld";
         else if (name_.find("PRO") != std::string::npos)
            field_ = "pro";
         else if (name_.find("NIT") != std::string::npos)
            field_ = "nit";
         else if (name_.find("LEU") != std::string::npos)
            field_ = "leu";
         else if (name_.find("GLU") != std::string::npos)
            field_ = "glu";
         else if (name_.find("SG") != std::string::npos)
            field_ = "sg";
         else if (name_.find("pH") != std::string::npos)
            field_ = "ph";
         else if (name_.find("Color") != std::string::npos)
            field_ = "color";
         else if (name_.find("Clarity") != std::string::npos)
            field_ = "clarity";


         _lisFields.push_back(new FieldsDirUiBcc3600(static_cast<int>(i), name_, field_, value_, rawdata_));
      }
   }

   return true;
}

} // namespace dirui
} // namespace tbs