#include <string>
#include "tobasalis/bci/record_vidas.h"

namespace tbs {
namespace bci {

VidasRecord::VidasRecord(const std::string& bciString)
   : Record(bciString)
{
   setRecordType(RecordType::Vidas);
   initFields();
   _totalField = (int)_bciFields.size(); // 20;
}

void VidasRecord::initFields()
{
   _bciFields.push_back(Record::createBciField("mt", "MessageType"));               // mt
   _bciFields.push_back(Record::createBciField("pi", "PatientId"));                 // pi
   _bciFields.push_back(Record::createBciField("pn", "PatientName"));               // pn
   _bciFields.push_back(Record::createBciField("pb", "PatientDob"));                // pb
   _bciFields.push_back(Record::createBciField("ps", "Gender"));                    // ps
   _bciFields.push_back(Record::createBciField("so", "SampleOrigine"));             // so
   _bciFields.push_back(Record::createBciField("si", "SpecimenSeparator"));         // si
   _bciFields.push_back(Record::createBciField("ci", "SampleId"));                  // ci
   _bciFields.push_back(Record::createBciField("rt", "ShortAssayName"));            // rt
   _bciFields.push_back(Record::createBciField("rn", "LongAssayName"));             // rn
   _bciFields.push_back(Record::createBciField("tt", "CompletedTime"));             // tt
   _bciFields.push_back(Record::createBciField("td", "CompletedDate"));             // td
   _bciFields.push_back(Record::createBciField("ql", "QualitativeResult"));         // ql
   _bciFields.push_back(Record::createBciField("qn", "QuantitativeResult"));        // qn
   _bciFields.push_back(Record::createBciField("y3", "UnitAssociatedWithQn"));      // y3
   _bciFields.push_back(Record::createBciField("qd", "Dilution"));                  // qd
   _bciFields.push_back(Record::createBciField("nc", "CalibrationExpired"));        // nc
   _bciFields.push_back(Record::createBciField("id", "InstrumentId"));              // id
   _bciFields.push_back(Record::createBciField("sn", "SerialNumber"));              // sn
   _bciFields.push_back(Record::createBciField("m4", "Technologist"));              // m4
}

} // namespace bci
} // namespace tbs