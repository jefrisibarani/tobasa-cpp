#include <string>
#include <tobasa/logger.h>
#include "tobasalis/bci/patientrecord_vitek2compact.h"

namespace tbs {
namespace bci {

PatientRecordVitek2Compact::PatientRecordVitek2Compact(const std::string& bciString)
   : Record(bciString)
{
   setRecordType(RecordType::Patient);
   initFields();
   _totalField = (int)_bciFields.size();
   _name = "PatientRecord";
}

void PatientRecordVitek2Compact::initFields()
{
   //Logger::logD("[lis_record] GeneralRecordVitek2Compact: Initializing Fields");

   _bciFields.push_back(Record::createBciField("pi", "PatientID"));                      // pi
   _bciFields.push_back(Record::createBciField("pv", "PatientAltID"));                   // pv
   _bciFields.push_back(Record::createBciField("pn", "PatientName"));                    // pn
   _bciFields.push_back(Record::createBciField("pl", "PatientLocationCode"));            // pl
   _bciFields.push_back(Record::createBciField("p2", "PatientLocationName"));            // p2
   _bciFields.push_back(Record::createBciField("pp", "PatientPhysicianCode"));           // pp
   _bciFields.push_back(Record::createBciField("p5", "PatientPhysicianName"));           // p5
   _bciFields.push_back(Record::createBciField("pc", "PatientCommentCode"));             // pc
   _bciFields.push_back(Record::createBciField("pt", "PatientCommentText"));             // pt
   _bciFields.push_back(Record::createBciField("w1", "PatientWildField1"));              // w1
}

} // namespace bci
} // namespace tbs