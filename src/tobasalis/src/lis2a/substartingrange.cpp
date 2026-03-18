#include "tobasalis/lis2a/substartingrange.h"

namespace tbs {
namespace lis2a {

StartingRange::StartingRange()
   : SubRecord()
{
   _totalField = 3;
   initFields();
}

void StartingRange::initFields()
{
   _lisFields.push_back(Record::createField(1, "PatientID"));
   _lisFields.push_back(Record::createField(2, "SpecimenID"));
   _lisFields.push_back(Record::createField(3, "Reserved"));
}

} // namespace lis2a
} // namespace tbs