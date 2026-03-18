#include "tobasalis/lis2a/subuniversaltest.h"

namespace tbs {
namespace lis2a {

UniversalTestID::UniversalTestID()
   : SubRecord()
{
   _totalField = 8;
   initFields();
}


void UniversalTestID::initFields()
{
   _lisFields.push_back(Record::createField(1, "TestID"));
   _lisFields.push_back(Record::createField(2, "TestName"));
   _lisFields.push_back(Record::createField(3, "TestType"));
   _lisFields.push_back(Record::createField(4, "ManufacturerCode"));
   _lisFields.push_back(Record::createField(5, "OptionalField1"));
   _lisFields.push_back(Record::createField(6, "OptionalField2"));
   _lisFields.push_back(Record::createField(7, "OptionalField3"));
   _lisFields.push_back(Record::createField(8, "OptionalField4"));
}

} // namespace lis2a
} // namespace tbs