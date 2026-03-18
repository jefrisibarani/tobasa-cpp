#pragma once

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * SpecimenRecord
 */
class SpecimenRecord
   : public Record
{
protected:
   virtual void initFields();

public:
   SpecimenRecord(const std::string& bciString);
   SpecimenRecord() = default;
   ~SpecimenRecord() = default;

   std::string specimenSeparator() { return getFieldValue("si"); }                        // si
   std::string relativeSpecimenNumber() { return getFieldValue("s0"); }                   // s0
   std::string specimenSourceCode() { return getFieldValue("ss"); }                       // ss
   std::string specimenSourceText() { return getFieldValue("s5"); }                       // s5
   std::string specimenSiteCode() { return getFieldValue("st"); }                         // st
   std::string specimenSiteText() { return getFieldValue("s6"); }                         // s6
   std::string specimenLocationCode() { return getFieldValue("sl"); }                     // sl
   std::string specimenLocationText() { return getFieldValue("s7"); }                     // s7
   std::string specimenRequestingPhysicianCode() { return getFieldValue("sp"); }          // sp
   std::string specimenRequestingPhysicianText() { return getFieldValue("s8"); }          // s8
   std::string specimenServiceCode() { return getFieldValue("sx"); }                      // sx
   std::string specimenServiceText() { return getFieldValue("s9"); }                      // s9
   std::string specimenStatusCode() { return getFieldValue("sy"); }                       // sy
   std::string specimenStatusText() { return getFieldValue("sz"); }                       // sz
   std::string specimenCollectionDate() { return getFieldValue("s1"); }                   // s1
   std::string specimenCollectionTime() { return getFieldValue("s2"); }                   // s2
   std::string specimenReceiptDate() { return getFieldValue("s3"); }                      // s3
   std::string specimenReceiptTime() { return getFieldValue("s4"); }                      // s4
   std::string patientTemperature() { return getFieldValue("sf"); }                       // sf
   std::string collectionNumber() { return getFieldValue("so"); }                         // so
   std::string specimenCommentCode() { return getFieldValue("sc"); }                      // sc
   std::string specimenCommentText() { return getFieldValue("sn"); }                      // sn
   std::string specimenUserField1Code() { return getFieldValue("ug"); }                   // ug
   std::string specimenUserField1Text() { return getFieldValue("uh"); }                   // uh
   std::string specimenUserField2Code() { return getFieldValue("ui"); }                   // ui
   std::string specimenUserField2Text() { return getFieldValue("uj"); }                   // uj
   std::string specimenUserField3Code() { return getFieldValue("uk"); }                   // uk
   std::string specimenUserField3Text() { return getFieldValue("ul"); }                   // ul
   std::string specimenUserField4Code() { return getFieldValue("um"); }                   // um
   std::string specimenUserField4Text() { return getFieldValue("un"); }                   // un
   std::string specimenUserField5Code() { return getFieldValue("uo"); }                   // uo
   std::string specimenUserField5Text() { return getFieldValue("up"); }                   // up
   std::string specimenUserField6Code() { return getFieldValue("uq"); }                   // ug
   std::string specimenUserField6Text() { return getFieldValue("ur"); }                   // ur
   std::string specimenUserField7Code() { return getFieldValue("us"); }                   // us
   std::string specimenUserField7Text() { return getFieldValue("ut"); }                   // ut
   std::string specimenUserField8Code() { return getFieldValue("uu"); }                   // uu
   std::string specimenUserField8Text() { return getFieldValue("uv"); }                   // uv
   std::string physicianAddressLine1() { return getFieldValue("sa"); }                    // sa
   std::string physicianAddressLine2() { return getFieldValue("sb"); }                    // sb
   std::string physicianAddressLine3() { return getFieldValue("se"); }                    // se
   std::string physicianAddressLine4() { return getFieldValue("sd"); }                    // sd
   std::string specimenWildField1() { return getFieldValue("wa"); }                       // wa
   std::string specimenWildField2() { return getFieldValue("wb"); }                       // wb
   std::string specimenWildField3() { return getFieldValue("wc"); }                       // wc
   std::string specimenWildField4() { return getFieldValue("wd"); }                       // wd
   std::string specimenWildField5() { return getFieldValue("we"); }                       // we

   void setSpecimenSeparator(const std::string& val) { setFieldValue("si", val); }                // si
   void setRelativeSpecimenNumber(const std::string& val) { setFieldValue("s0", val); }           // s0
   void setSpecimenSourceCode(const std::string& val) { setFieldValue("ss", val); }               // ss
   void setSpecimenSourceText(const std::string& val) { setFieldValue("s5", val); }               // s5
   void setSpecimenSiteCode(const std::string& val) { setFieldValue("st", val); }                 // st
   void setSpecimenSiteText(const std::string& val) { setFieldValue("s6", val); }                 // s6
   void setSpecimenLocationCode(const std::string& val) { setFieldValue("sl", val); }             // sl
   void setSpecimenLocationText(const std::string& val) { setFieldValue("s7", val); }             // s7
   void setSpecimenRequestingPhysicianCode(const std::string& val) { setFieldValue("sp", val); }  // sp
   void setSpecimenRequestingPhysicianText(const std::string& val) { setFieldValue("s8", val); }  // s8
   void setSpecimenServiceCode(const std::string& val) { setFieldValue("sx", val); }              // sx
   void setSpecimenServiceText(const std::string& val) { setFieldValue("s9", val); }              // s9
   void setSpecimenStatusCode(const std::string& val) { setFieldValue("sy", val); }               // sy
   void setSpecimenStatusText(const std::string& val) { setFieldValue("sz", val); }               // sz
   void setSpecimenCollectionDate(const std::string& val) { setFieldValue("s1", val); }           // s1
   void setSpecimenCollectionTime(const std::string& val) { setFieldValue("s2", val); }           // s2
   void setSpecimenReceiptDate(const std::string& val) { setFieldValue("s3", val); }              // s3
   void setSpecimenReceiptTime(const std::string& val) { setFieldValue("s4", val); }              // s4
   void setPatientTemperature(const std::string& val) { setFieldValue("sf", val); }               // sf
   void setCollectionNumber(const std::string& val) { setFieldValue("so", val); }                 // so
   void setSpecimenCommentCode(const std::string& val) { setFieldValue("sc", val); }              // sc
   void setSpecimenCommentText(const std::string& val) { setFieldValue("sn", val); }              // sn
   void setSpecimenUserField1Code(const std::string& val) { setFieldValue("ug", val); }           // ug
   void setSpecimenUserField1Text(const std::string& val) { setFieldValue("uh", val); }           // uh
   void setSpecimenUserField2Code(const std::string& val) { setFieldValue("ui", val); }           // ui
   void setSpecimenUserField2Text(const std::string& val) { setFieldValue("uj", val); }           // uj
   void setSpecimenUserField3Code(const std::string& val) { setFieldValue("uk", val); }           // uk
   void setSpecimenUserField3Text(const std::string& val) { setFieldValue("ul", val); }           // ul
   void setSpecimenUserField4Code(const std::string& val) { setFieldValue("um", val); }           // um
   void setSpecimenUserField4Text(const std::string& val) { setFieldValue("un", val); }           // un
   void setSpecimenUserField5Code(const std::string& val) { setFieldValue("uo", val); }           // uo
   void setSpecimenUserField5Text(const std::string& val) { setFieldValue("up", val); }           // up
   void setSpecimenUserField6Code(const std::string& val) { setFieldValue("uq", val); }           // ug
   void setSpecimenUserField6Text(const std::string& val) { setFieldValue("ur", val); }           // ur
   void setSpecimenUserField7Code(const std::string& val) { setFieldValue("us", val); }           // us
   void setSpecimenUserField7Text(const std::string& val) { setFieldValue("ut", val); }           // ut
   void setSpecimenUserField8Code(const std::string& val) { setFieldValue("uu", val); }           // uu
   void setSpecimenUserField8Text(const std::string& val) { setFieldValue("uv", val); }           // uv
   void setPhysicianAddressLine1(const std::string& val) { setFieldValue("sa", val); }            // sa
   void setPhysicianAddressLine2(const std::string& val) { setFieldValue("sb", val); }            // sb
   void setPhysicianAddressLine3(const std::string& val) { setFieldValue("se", val); }            // se
   void setPhysicianAddressLine4(const std::string& val) { setFieldValue("sd", val); }            // sd
   void setSpecimenWildField1(const std::string& val) { setFieldValue("wa", val); }               // wa
   void setSpecimenWildField2(const std::string& val) { setFieldValue("wb", val); }               // wb
   void setSpecimenWildField3(const std::string& val) { setFieldValue("wc", val); }               // wc
   void setSpecimenWildField4(const std::string& val) { setFieldValue("wd", val); }               // wd
   void setSpecimenWildField5(const std::string& val) { setFieldValue("we", val); }               // we
};

} // namespace bci
} // namespace tbs 