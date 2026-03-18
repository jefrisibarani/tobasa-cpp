#include <utility>
#include <string>
#include <tobasa/logger.h>
#include <tobasa/util_string.h>
#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

class RecordFields
{
private:
   std::vector<std::string> _items;
   std::string _value;
   friend class Record;

public:
   RecordFields(const std::string& bciString, char delimiterChar);
   std::string getValue() { return _value; }
   void setValue(const std::string& value) { _value = value; }
   std::string getField(int indx);
   std::string getField(const std::string& code);
};

RecordFields::RecordFields(const std::string& lisString, char delimiterChar)
{
   _value = lisString;
   _items = tbs::util::split(lisString, delimiterChar);
}

std::string RecordFields::getField(int indx)
{
   if (_items.size() < indx)
      return "";

   std::string result = _items[indx - 1];
   return result;
}

std::string RecordFields::getField(const std::string& code)
{
   for (size_t i=0;i<_items.size();i++)
   {
      std::string field = _items.at(i);
      if (field.find(code) == 0)
         return field;
   }

   return "";
}


/** Record **/

/*static*/
char Record::FieldDelimiter = '|';

/*static*/
BciField Record::createBciField(int index, const std::string& name, const std::string& value)
{
   BciField field(index, name, value);
   return field;
}

/*static*/
BciField Record::createBciField(const std::string& code, const std::string& name, const std::string& value)
{
   BciField field(code, name, value);
   return field;
}

std::string Record::get(const std::string& fieldName)
{
   std::vector<BciField>::iterator it;
   for (it = _bciFields.begin(); it != _bciFields.end(); ++it)
   {
      BciField& field = *it;
      if (field.name() == fieldName)
         return field.value();
   }

   return "";
}

void Record::set(const std::string& fieldName, const std::string& value)
{
   std::vector<BciField>::iterator it;
   for (it = _bciFields.begin(); it != _bciFields.end(); ++it)
   {
      BciField& field = *it;
      if (field.name() == fieldName)
      {
         field.setValue(value);
         return;
      }
   }
}

std::string Record::getFieldValue(int index)
{
   std::vector<BciField>::iterator it;
   for (it = _bciFields.begin(); it != _bciFields.end(); ++it)
   {
      BciField& field = *it;
      if (field.index() == index)
         return field.value();
   }

   return "";
}

void Record::setFieldValue(int index, const std::string& val)
{
   std::vector<BciField>::iterator it;
   for (it = _bciFields.begin(); it != _bciFields.end(); ++it)
   {
      BciField& field = *it;
      if (field.index() == index)
      {
         field.setValue(val);
         return;
      }
   }
}

std::string Record::getFieldValue(const std::string& code)
{
   std::vector<BciField>::iterator it;
   for (it = _bciFields.begin(); it != _bciFields.end(); ++it)
   {
      BciField& field = *it;
      if (field.code() == code)
         return field.value();
   }

   return "";
}

void Record::setFieldValue(const std::string& code, const std::string& val)
{
   std::vector<BciField>::iterator it;
   for (it = _bciFields.begin(); it != _bciFields.end(); ++it)
   {
      BciField& field = *it;
      if (field.code() == code)
      {
         field.setValue(val);
         return;
      }
   }
}

Record::Record(const std::string& lisString)
   : lis::Record(lisString)
{
   _recordType = RecordType::Unknown;
}

/*
mtrsl|piMARY ANNA|pnMARY ANNA|pb|ps|so|si|ciMARY ANNA|rtHBS|rnHBs Ag Ultra|tt14:20|td26/05/2017|qlNegative|qn0.00|qd1|ncvalid|idVIDASPC01|sn|m4LabAdmin|
*/
bool Record::fromString(const std::string& bciString)
{
   if (bciString.empty() && _lisString.empty())
      return false;

   if (_lisString.empty())
      _lisString = bciString;

   char sepChar = Record::FieldDelimiter;

   // Tokenize lisString into array stored in RecordFields
   RecordFields rf(_lisString, sepChar);

   if (rf._items.size() != _totalField)
   {
      Logger::logD("[lis_record] {}: missmatch feld size in parsed raw data, expected {}, got {} fields", recordTypeStr(), _totalField, rf._items.size());
      return false;
   }

   if (_bciFields.size() != _totalField)
   {
      Logger::logD("[lis_record] {}: missmatch field size in cached fields, expected {}, got {} fields", recordTypeStr(), _totalField, _bciFields.size());
      return false;
   }

   int i = 0;
   std::vector<BciField>::iterator it;
   for (it = _bciFields.begin(); it != _bciFields.end(); ++it)
   {
      ++i;
      BciField& field = *it;
      std::string fieldStr = rf.getField(field.code());
      fieldStr = tbs::util::trim(fieldStr);

      if (!fieldStr.empty())
      {
         // the first two char is field code, remove!
         std::string code = fieldStr.substr(0, 2);
         std::string value = fieldStr.substr(2);
         // Compare code
         if (field.code() == code)
            field.setValue(value);
      }
   }

   return true;
}

std::string Record::toString()
{
   char sepChar = Record::FieldDelimiter;
   std::string output;
   int idx = 0;
   int totalField = (int)_bciFields.size();

   std::vector<BciField>::iterator it;
   for (it = _bciFields.begin(); it != _bciFields.end(); ++it)
   {
      ++idx;
      BciField& field = *it;

      std::string fieldStr = field.value();

      output += field.code();
      output += fieldStr;
      output += sepChar;
   }

   return output;
}

/*static*/
std::string Record::createBciStringFromArray(std::vector<std::string>* arr)
{
   char sepChar = Record::FieldDelimiter;
   std::string output;
   int totalField = (int)arr->size();

   for (int i = 0; i < totalField; i++)
   {
      std::string fieldStr = arr->at(i);

      if (!output.empty()) {
         output += sepChar;
      }

      output += fieldStr;
   }

   return output;
}

std::string Record::recordTypeStr()
{
   switch (_recordType)
   {
   case tbs::bci::RecordType::General:
      return "General";
      break;
   case tbs::bci::RecordType::Patient:
      return "Patient";
      break;
   case tbs::bci::RecordType::Specimen:
      return "Specimen";
      break;
   case tbs::bci::RecordType::Culture:
      return "Culture";
      break;
   case tbs::bci::RecordType::Test:
      return "Test";
      break;
   case tbs::bci::RecordType::Result:
      return "Result";
      break;
   case tbs::bci::RecordType::Af:
      return "Af";
      break;
   case tbs::bci::RecordType::Ap:
      return "Ap";
      break;
   case tbs::bci::RecordType::Vidas:
      return "Vidas";
      break;
   default:
      return "Unknown";
      break;
   }
}

} // namespace bci
} // namespace tbs