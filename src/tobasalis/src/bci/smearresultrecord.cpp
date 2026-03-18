#include <string>
#include "tobasalis/bci/smearresultrecord.h"

namespace tbs {
namespace bci {

SmearResultRecord::SmearResultRecord(const std::string& bciString)
   : Record(bciString)
{
   setRecordType(RecordType::Result);
   initFields();
   _totalField = (int)_bciFields.size(); // 5
}

void SmearResultRecord::initFields()
{
   _bciFields.push_back(Record::createBciField("ra", "ResultSeparator"));    // ra
   _bciFields.push_back(Record::createBciField("rs", "SmearCode"));          // rs
   _bciFields.push_back(Record::createBciField("rm", "SmearName"));          // rm
   _bciFields.push_back(Record::createBciField("o4", "FinalModifierCode"));  // o4
   _bciFields.push_back(Record::createBciField("o5", "FinalModifierName"));  // o5
}

} // namespace bci
} // namespace tbs