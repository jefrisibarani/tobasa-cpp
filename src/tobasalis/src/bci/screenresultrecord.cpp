#include <string>
#include "tobasalis/bci/screenresultrecord.h"

namespace tbs {
namespace bci {

ScreenResultRecord::ScreenResultRecord(const std::string& bciString)
   : Record(bciString)
{
   setRecordType(RecordType::Result);
   initFields();
   _totalField = (int)_bciFields.size(); // 10
}

void ScreenResultRecord::initFields()
{
   _bciFields.push_back(Record::createBciField("ra", "ResultSeparator"));      // ra
   _bciFields.push_back(Record::createBciField("o1", "FinalOrganismCode"));    // o1
   _bciFields.push_back(Record::createBciField("o2", "FinalOrganismName"));    // o2
   _bciFields.push_back(Record::createBciField("o4", "FinalModifierCode"));    // o4
   _bciFields.push_back(Record::createBciField("o5", "FinalModifierName"));    // o5
   _bciFields.push_back(Record::createBciField("o6", "OrganismGroupCode"));    // o6
   _bciFields.push_back(Record::createBciField("o7", "OrganismGroupName("));   // o7
   _bciFields.push_back(Record::createBciField("o8", "TotalCountText"));       // o8
   _bciFields.push_back(Record::createBciField("o3", "TotalCountCode"));       // o3
   _bciFields.push_back(Record::createBciField("rh", "HoursToPositive"));      // rh
}

} // namespace bci
} // namespace tbs