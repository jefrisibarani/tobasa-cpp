#include <string>
#include "tobasalis/bci/specimenrecord.h"

namespace tbs {
namespace bci {

SpecimenRecord::SpecimenRecord(const std::string& bciString)
   : Record(bciString)
{
   setRecordType(RecordType::Specimen);
   initFields();
   _totalField = (int)_bciFields.size(); // 47
}

void SpecimenRecord::initFields()
{
   _bciFields.push_back(Record::createBciField("si", "SpecimenSeparator"));                 // si
   _bciFields.push_back(Record::createBciField("s0", "RelativeSpecimenNumber"));            // s0
   _bciFields.push_back(Record::createBciField("ss", "SpecimenSourceCode"));                // ss
   _bciFields.push_back(Record::createBciField("s5", "SpecimenSourceText"));                // s5
   _bciFields.push_back(Record::createBciField("st", "SpecimenSiteCode"));                  // st
   _bciFields.push_back(Record::createBciField("s6", "SpecimenSiteText"));                  // s6
   _bciFields.push_back(Record::createBciField("sl", "SpecimenLocationCode"));              // sl
   _bciFields.push_back(Record::createBciField("s7", "SpecimenLocationText"));              // s7
   _bciFields.push_back(Record::createBciField("sp", "SpecimenRequestingPhysicianCode"));   // sp
   _bciFields.push_back(Record::createBciField("s8", "SpecimenRequestingPhysicianText"));   // s8
   _bciFields.push_back(Record::createBciField("sx", "SpecimenServiceCode"));               // sx
   _bciFields.push_back(Record::createBciField("s9", "SpecimenServiceText"));               // s9
   _bciFields.push_back(Record::createBciField("sy", "SpecimenStatusCode"));                // sy
   _bciFields.push_back(Record::createBciField("sz", "SpecimenStatusText"));                // sz
   _bciFields.push_back(Record::createBciField("s1", "SpecimenCollectionDate"));            // s1
   _bciFields.push_back(Record::createBciField("s2", "SpecimenCollectionTime"));            // s2
   _bciFields.push_back(Record::createBciField("s3", "SpecimenReceiptDate"));               // s3
   _bciFields.push_back(Record::createBciField("s4", "SpecimenReceiptTime"));               // s4
   _bciFields.push_back(Record::createBciField("sf", "PatientTemperature"));                // sf
   _bciFields.push_back(Record::createBciField("so", "CollectionNumber"));                  // so
   _bciFields.push_back(Record::createBciField("sc", "SpecimenCommentCode"));               // sc
   _bciFields.push_back(Record::createBciField("sn", "SpecimenCommentText"));               // sn
   _bciFields.push_back(Record::createBciField("ug", "SpecimenUserField1Code"));            // ug
   _bciFields.push_back(Record::createBciField("uh", "SpecimenUserField1Text"));            // uh
   _bciFields.push_back(Record::createBciField("ui", "SpecimenUserField2Code"));            // ui
   _bciFields.push_back(Record::createBciField("uj", "SpecimenUserField2Text"));            // uj
   _bciFields.push_back(Record::createBciField("uk", "SpecimenUserField3Code"));            // uk
   _bciFields.push_back(Record::createBciField("ul", "SpecimenUserField3Text"));            // ul
   _bciFields.push_back(Record::createBciField("um", "SpecimenUserField4Code"));            // um
   _bciFields.push_back(Record::createBciField("un", "SpecimenUserField4Text"));            // un
   _bciFields.push_back(Record::createBciField("uo", "SpecimenUserField5Code"));            // uo
   _bciFields.push_back(Record::createBciField("up", "SpecimenUserField5Text"));            // up
   _bciFields.push_back(Record::createBciField("uq", "SpecimenUserField6Code"));            // uq
   _bciFields.push_back(Record::createBciField("ur", "SpecimenUserField6Text"));            // ur
   _bciFields.push_back(Record::createBciField("us", "SpecimenUserField7Code"));            // us
   _bciFields.push_back(Record::createBciField("ut", "SpecimenUserField7Text"));            // ut
   _bciFields.push_back(Record::createBciField("uu", "SpecimenUserField8Code"));            // uu
   _bciFields.push_back(Record::createBciField("uv", "SpecimenUserField8Text"));            // uv
   _bciFields.push_back(Record::createBciField("sa", "PhysicianAddressLine1"));             // sa
   _bciFields.push_back(Record::createBciField("sb", "PhysicianAddressLine2"));             // sb
   _bciFields.push_back(Record::createBciField("se", "PhysicianAddressLine3"));             // se
   _bciFields.push_back(Record::createBciField("sd", "PhysicianAddressLine4"));             // sd
   _bciFields.push_back(Record::createBciField("wa", "SpecimenWildField1"));                // wa
   _bciFields.push_back(Record::createBciField("wb", "SpecimenWildField2"));                // wb
   _bciFields.push_back(Record::createBciField("pi", "SpecimenWildField3"));                // wc
   _bciFields.push_back(Record::createBciField("wc", "SpecimenWildField4"));                // wd
   _bciFields.push_back(Record::createBciField("we", "SpecimenWildField5"));                // we
}

} // namespace bci
} // namespace tbs