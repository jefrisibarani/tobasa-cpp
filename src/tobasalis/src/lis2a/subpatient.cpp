#include <string>
#include "tobasalis/lis2a/subpatient.h"

namespace tbs {
namespace lis2a {

PatientName::PatientName()
   : SubRecord()
{
   _totalField = 5;
   initFields();
}

void PatientName::initFields()
{
   _lisFields.push_back(Record::createField(1, "LastName"));
   _lisFields.push_back(Record::createField(2, "FirstName"));
   _lisFields.push_back(Record::createField(3, "MiddleName"));
   _lisFields.push_back(Record::createField(4, "Suffix"));
   _lisFields.push_back(Record::createField(5, "Title"));
}

} // namespace lis2a
} // namespace tbs