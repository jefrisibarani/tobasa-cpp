#pragma once

#include <memory>
#include <vector>
#include "tobasalis/lis/record.h"
#include "tobasalis/bci/field.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * RecordType
 */
enum class RecordType
{
   General,
   Patient,
   Specimen,
   Culture,
   Test,
   Result,
   Af,     // VITEK2 Compact
   Ap,     // VITEK2 Compact
   Vidas,
   Unknown
};

/** \ingroup LIS
 * Record
 */
class Record
   : public tbs::lis::Record
{
public:
   static char FieldDelimiter;

   Record(const std::string& lisString = "");
   virtual ~Record() = default;

   bool fromString(const std::string& lisString = "");
   virtual std::string toString();
   RecordType recordType() { return _recordType; }
   std::string recordTypeStr();

   std::string get(const std::string& fieldName);
   void set(const std::string& fieldName, const std::string& value);
   std::string name() { return _name; }

   static std::string createBciStringFromArray(std::vector<std::string>* arr);

protected:
   RecordType            _recordType;
   std::string           _name;
   std::vector<BciField> _bciFields;

   virtual void initFields() {}
   static BciField createBciField(int index, const std::string& name, const std::string& value = "");
   static BciField createBciField(const std::string& code, const std::string& name, const std::string& value = "");
   std::vector<BciField>& getFields() { return _bciFields; }
   void setRecordType(RecordType type) { _recordType = type; }
   std::string getFieldValue(int index);
   void setFieldValue(int index, const std::string& val);
   std::string getFieldValue(const std::string& code);
   void setFieldValue(const std::string& code, const std::string& val);
};

} // namespace bci
} // namespace tbs 