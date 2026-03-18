#include <string>
#include "tobasalis/bci/culturerecord.h"

namespace tbs {
namespace bci {

CultureRecord::CultureRecord(const std::string& bciString)
   : Record(bciString)
{
   setRecordType(RecordType::Culture);
   initFields();
   _totalField = (int)_bciFields.size(); // 12
}

void CultureRecord::initFields()
{
   _bciFields.push_back(Record::createBciField("ci", "ExaminationID"));          // ci
   _bciFields.push_back(Record::createBciField("c0", "ExaminationNumber"));      // c0
   _bciFields.push_back(Record::createBciField("ct", "CultureTypeCode"));        // ct
   _bciFields.push_back(Record::createBciField("cn", "CultureTypeText"));        // cn
   _bciFields.push_back(Record::createBciField("c1", "CultureCompletionDate"));  // c1
   _bciFields.push_back(Record::createBciField("c2", "CultureCompletionTime"));  // c2
   _bciFields.push_back(Record::createBciField("c3", "CultureStatusCode"));      // c3
   _bciFields.push_back(Record::createBciField("c4", "CultureStatusText"));      // c4
   _bciFields.push_back(Record::createBciField("x1", "CultureWildField1"));      // x1
   _bciFields.push_back(Record::createBciField("x2", "CultureWildField2"));      // x2
   _bciFields.push_back(Record::createBciField("na", "CultureCommandCode"));     // na
   _bciFields.push_back(Record::createBciField("nb", "CultureCommandText"));     // nb
}

} // namespace bci
} // namespace tbs