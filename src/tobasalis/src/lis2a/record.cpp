#include <utility>
#include <string>
#include <tobasa/util_string.h>
#include <tobasa/logger.h>
#include "tobasalis/lis2a/subrecord.h"
#include "tobasalis/lis2a/record.h"
#include "tobasalis/lis2a/field.h"

namespace tbs {
namespace lis2a {

std::string getValueFromVector(const std::vector<std::string>& vector, int idx)
{
   if (vector.size() < idx)
      return "";

   std::string val = vector[idx - 1];
   return val;
}

/*static*/
LisField Record::createField(int index, const std::string& name, const std::string& value)
{
   LisField field(index, name, value);
   return field;
}

Record::Record(const std::string& lisString)
   : lis::Record(lisString)
{
   _recordType = RecordType::Unknown;
   _isSubRecord = false;

   // set delimiter default value
   _delimiter.fieldDelimiter     = '|';
   _delimiter.repeatDelimiter    = '\\';
   _delimiter.componentDelimiter = '!';
   _delimiter.escapeCharacter    = '~';
}

std::string Record::getFieldValue(int index)
{
   std::vector<LisField>::iterator it;
   for (it = _lisFields.begin(); it != _lisFields.end(); ++it)
   {
      LisField& field = *it;
      if (field.index() == index)
         return field.value();
   }
   return "";
}

void Record::setFieldValue(int index, const std::string& val)
{
   std::vector<LisField>::iterator it;
   for (it = _lisFields.begin(); it != _lisFields.end(); ++it)
   {
      LisField& field = *it;
      if (field.index() == index)
      {
         field.setValue(val);
         return;
      }
   }
}

std::string Record::removeOptionalSubFields(const std::string& string)
{
   if ( string.find(componentDelimiter()) )
   {
      std::vector<std::string> ss = tbs::util::split(string, componentDelimiter());
      return ss[0];
   }
   else
      return string;
}


std::string Record::unescapeString(const std::string& string)
{
   std::string value = string;

   std::string escapeChar(1,     escapeCharacter() );
   std::string fieldDelim(1,     fieldDelimiter() );
   std::string componentDelim(1, componentDelimiter() );
   std::string repeatDelim(1,    repeatDelimiter() );

   value = tbs::util::replace(value, escapeChar + "F" + escapeChar, fieldDelim);
   value = tbs::util::replace(value, escapeChar + "S" + escapeChar, componentDelim);
   value = tbs::util::replace(value, escapeChar + "R" + escapeChar, repeatDelim);
   value = tbs::util::replace(value, escapeChar + "E" + escapeChar, escapeChar);

   return value;
}

std::string Record::escapeString(const std::string& string, bool subrecord)
{
   std::string escapeChar(1,     escapeCharacter() );
   std::string fieldDelim(1,     fieldDelimiter() );
   std::string componentDelim(1, componentDelimiter() );

   std::string value = string;

   value = tbs::util::replace(value, escapeChar, escapeChar + "E" + escapeChar);
   value = tbs::util::replace(value, fieldDelim, escapeChar + "F" + escapeChar);

   if (subrecord)
      value = tbs::util::replace(value, componentDelim, escapeChar + "S" + escapeChar);

   return value;
}

bool Record::fromString(const std::string& lisString)
{
   if (lisString.empty() && _lisString.empty())
      return false;

   if (_lisString.empty())
      _lisString = lisString;

   char sepChar = (!isSubRecord()) ? fieldDelimiter() : componentDelimiter();

  	//Logger::logT("[lis_record] {}: fromString: {}", recordTypeStr(), _lisString);

   // Tokenize lisString into recordFields vector
   std::vector<std::string> recordFields = tbs::util::split(_lisString, sepChar);
   
   /* debug only
   if ( isSubRecord() )
   {
      int tot = getTotalField();
      int size = (int)getFields().size();
      Logger::logD("Record: fromString: _subrecord_ total: {}, size: {} ", tot, size);
   }
   */

   if (recordFields.size() != _totalField)
      Logger::logT("[lis_record] {}: missmatch field size in parsed raw data, expected {}, got {} fields", recordTypeStr(), _totalField, recordFields.size());

   if (_lisFields.size() != _totalField)
      Logger::logT("[lis_record] {}: missmatch field size in cached fields, expected {}, got {} fields", recordTypeStr(), _totalField, _lisFields.size());

   int i = 0;
   std::vector<LisField>::iterator it;
   for (it = _lisFields.begin(); it != _lisFields.end(); ++it)
   {
      ++i;
      LisField& field = *it;
      std::string fieldStr = getValueFromVector( recordFields, field.index() );

      if ( !fieldStr.empty() )
      {
         fieldStr = unescapeString(fieldStr);

         switch (field.valueType())
         {
         case FieldValueType::Integer:
            field.setValue(fieldStr);
         break;
         case FieldValueType::String:
            field.setValue(fieldStr);
         break;
         case FieldValueType::DateTime:
            field.setValue(fieldStr);
         break;
         case FieldValueType::Enum:
            field.setValue(fieldStr);
         break;
         case FieldValueType::SubRecord:
         {
            Record* record = field.getSubRecord();
            if (record != nullptr) 
            {
               record->delimiter(_delimiter);
               record->fromString(fieldStr);
            }
            else 
               Logger::logE("[lis_record] {}: Cannot create sub record", recordTypeStr());

            field.setValue(fieldStr);
         }
         break;
         default:
            Logger::logE("[lis_record] {}: The LIS2-A2 String was not of the correct format", recordTypeStr());
            break;
         }
      }
   }
   return true;
}

std::string Record::toString()
{
   //Logger::logT("[lis_record] {}: toString", recordTypeStr());

   char sepChar = (!isSubRecord()) ? fieldDelimiter() : componentDelimiter();

   std::string output;
   int idx = 0;
   int totalField = (int)_lisFields.size();
   std::vector<LisField>::iterator it;
   for (it = _lisFields.begin(); it != _lisFields.end(); ++it)
   {
      ++idx;
      LisField& field = *it;

      std::string fieldStr = field.value();
      switch (field.valueType())
      {
      case FieldValueType::Integer:
         break;
      case FieldValueType::String:
         break;
      case FieldValueType::DateTime:
         break;
      case FieldValueType::Enum:
         break;
      case FieldValueType::SubRecord:
      {
         Record* record = field.getSubRecord();
         if (record != nullptr)
         {
            record->delimiter(_delimiter);
            fieldStr = record->toString();
         }
      }
      break;

      default:
         break;
      }

      if (!fieldStr.empty())
      {
         if (_recordType == RecordType::Header && idx < 3)
            output.append(fieldStr);
         else
            output.append(escapeString(fieldStr, _isSubRecord));
      }

      if (idx < totalField) 
         output.append(1, sepChar);

   }

   // strip unnecessary separator after last non empty field value
   // eg: transform   !!!GLUCOSE!!!!  into  !!!GLUCOSE
   util::removeTraillingChar(output, sepChar);

   // Note: LIS2-A2 uses CR as record separator
   if (!_isSubRecord) {
      output.append(1, '\r');
   }

   return output;
}

std::string Record::recordTypeStr()
{
   switch (_recordType)
   {
   case tbs::lis2a::RecordType::Header:
      return "Header";
      break;
   case tbs::lis2a::RecordType::Patient:
      return "Patient";
      break;
   case tbs::lis2a::RecordType::Order:
      return "Order";
      break;
   case tbs::lis2a::RecordType::Result:
      return "Result";
      break;
   case tbs::lis2a::RecordType::Comment:
      return "Comment";
      break;
   case tbs::lis2a::RecordType::Query:
      return "Query";
      break;
   case tbs::lis2a::RecordType::Terminator:
      return "Terminator";
      break;
   case tbs::lis2a::RecordType::Scientific:
      return "Scientific";
      break;
   case tbs::lis2a::RecordType::Information:
      return "Information";
      break;
   case tbs::lis2a::RecordType::SubRecord:
      return "SubRecord";
      break;
   case tbs::lis2a::RecordType::Unknown:
      return "Unknown";
      break;
   case tbs::lis2a::RecordType::DirUiH500:
      return "DirUiH500";
      break;
   case tbs::lis2a::RecordType::DirUiBcc3600:
      return "DirUiBcc3600";
      break;
   default:
      return "Unknown";
      break;
   }
}

} // namespace lis2a
} // namespace tbs