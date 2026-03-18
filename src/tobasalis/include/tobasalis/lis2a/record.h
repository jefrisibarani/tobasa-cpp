#pragma once

#include <memory>
#include <vector>
#include "tobasalis/lis/delimiter.h"
#include "tobasalis/lis/record.h"
#include "tobasalis/lis2a/field.h"


namespace tbs {
/** \ingroup LIS
  * CLSI LIS02-A2 (formerly ASTM 1394-97) Record
 */
namespace lis2a {

/** \ingroup LIS
 * RecordType
 */
enum class RecordType
{
   Header,
   Patient,
   Order,
   Result,
   Comment,
   Query,
   Terminator,
   Scientific,
   Information,
   SubRecord,
   Unknown,
   DirUiH500,
   DirUiBcc3600
};

/** \ingroup LIS
 * CLSI LIS2-A2 Base Record.
 *
 * Header Record
 * Patient Information Record
 * Test Order (Patient) Record
 * Test Order (Quality Control) Record
 * Result Record
 * Manufacturer Record
 * Comment Record
 * Message Terminator Record
 */
class Record
   : public lis::Record
{
public:

   Record(const std::string& lisString = "");
   virtual ~Record() = default;

   bool fromString(const std::string& lisString = "");
   virtual std::string toString();
   RecordType recordType() { return _recordType; }
   std::string recordTypeStr();

   char fieldDelimiter()               { return _delimiter.fieldDelimiter; }
   char repeatDelimiter()              { return _delimiter.repeatDelimiter; }
   char componentDelimiter()           { return _delimiter.componentDelimiter; }
   char escapeCharacter()              { return _delimiter.escapeCharacter; }
   
   void fieldDelimiter(char ch)        { _delimiter.fieldDelimiter     = ch;}
   void repeatDelimiter(char ch)       { _delimiter.repeatDelimiter    = ch;}
   void componentDelimiter(char ch)    { _delimiter.componentDelimiter = ch;}
   void escapeCharacter(char ch)       { _delimiter.escapeCharacter    = ch;}
   
   void delimiter(lis::Delimiter delimiter) { _delimiter = delimiter;}
   lis::Delimiter delimiter() { return _delimiter; }
   
protected:
   RecordType            _recordType;
   bool                  _isSubRecord;
   std::vector<LisField> _lisFields;
   lis::Delimiter        _delimiter;

   virtual void initFields() {}
   
   static LisField createField(int index, const std::string& name, const std::string& value = "");
   std::vector<LisField>& getFields() { return _lisFields; }
   void setRecordType(RecordType type) { _recordType = type; }
   std::string getFieldValue(int index);
   void setFieldValue(int index, const std::string& val);
   bool isSubRecord() { return _isSubRecord; }

   std::string removeOptionalSubFields(const std::string& string);
   std::string unescapeString(const std::string& string);
   std::string escapeString(const std::string& string, bool subrecord);
};

using RecordPtr = std::shared_ptr<lis2a::Record>;

} // namespace lis2a
} // namespace tbs 