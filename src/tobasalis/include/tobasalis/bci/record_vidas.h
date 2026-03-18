#pragma once

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * VidasRecord
 */
class VidasRecord
   : public Record
{
protected:
   virtual void initFields();

public:
   VidasRecord(const std::string& bciString);
   VidasRecord() = default;
   ~VidasRecord() = default;

   std::string messageType() { return getFieldValue("mt"); }               // mt
   std::string patientId() { return getFieldValue("pi"); }                 // pi
   std::string patientName() { return getFieldValue("pn"); }               // pn
   std::string patientDob() { return getFieldValue("pb"); }                // pb
   std::string gender() { return getFieldValue("ps"); }                    // ps
   std::string sampleOrigine() { return getFieldValue("so"); }             // so
   std::string specimenSeparator() { return getFieldValue("si"); }         // si
   std::string sampleId() { return getFieldValue("ci"); }                  // ci
   std::string shortAssayName() { return getFieldValue("rt"); }            // rt
   std::string longAssayName() { return getFieldValue("rn"); }             // rn
   std::string completedTime() { return getFieldValue("tt"); }             // tt
   std::string completedDate() { return getFieldValue("td"); }             // td
   std::string qualitativeResult() { return getFieldValue("ql"); }         // ql
   std::string quantitativeResult() { return getFieldValue("qn"); }        // qn
   std::string unitAssociatedWithQn() { return getFieldValue("y3"); }      // y3
   std::string dilution() { return getFieldValue("qd"); }                  // qd
   std::string calibrationExpired() { return getFieldValue("nc"); }        // nc
   std::string instrumentId() { return getFieldValue("id"); }              // id
   std::string serialNumber() { return getFieldValue("sn"); }              // sn
   std::string technologist() { return getFieldValue("m4"); }              // m4


   void setMessageType(const std::string& val) { setFieldValue("mt", val); }               // mt
   void setPatientId(const std::string& val) { setFieldValue("pi", val); }                 // pi
   void setPatientName(const std::string& val) { setFieldValue("pn", val); }               // pn
   void setPatientDob(const std::string& val) { setFieldValue("pb", val); }                // pb
   void setGender(const std::string& val) { setFieldValue("ps", val); }                    // ps
   void setSampleOrigine(const std::string& val) { setFieldValue("so", val); }             // so
   void setSpecimenSeparator(const std::string& val) { setFieldValue("si", val); }         // si
   void setSampleId(const std::string& val) { setFieldValue("ci", val); }                  // ci
   void setShortAssayName(const std::string& val) { setFieldValue("rt", val); }            // rt
   void setLongAssayName(const std::string& val) { setFieldValue("rn", val); }             // rn
   void setCompletedTime(const std::string& val) { setFieldValue("tt", val); }             // tt
   void setCompletedDate(const std::string& val) { setFieldValue("td", val); }             // td
   void setQualitativeResult(const std::string& val) { setFieldValue("ql", val); }         // ql
   void setQuantitativeResult(const std::string& val) { setFieldValue("qn", val); }        // qn
   void setUnitAssociatedWithQn(const std::string& val) { setFieldValue("y3", val); }      // y3
   void setDilution(const std::string& val) { setFieldValue("qd", val); }                  // qd
   void setCalibrationExpired(const std::string& val) { setFieldValue("nc", val); }        // nc
   void setInstrumentId(const std::string& val) { setFieldValue("id", val); }              // id
   void setSerialNumber(const std::string& val) { setFieldValue("sn", val); }              // sn
   void setTechnologist(const std::string& val) { setFieldValue("m4", val); }              // m4
};

} // namespace bci
} // namespace tbs 