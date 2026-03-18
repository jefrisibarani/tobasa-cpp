#pragma once

#include <string>
#include <vector>
#include <tobasa/json.h>

namespace tbs {
namespace lis {
namespace dto {

// OBX
struct HL7Obx
{
   long obxId   = 0;
   long dbObrId = 0;
   long dbPidId = 0;
   std::string name;
   std::string value;
   std::string unit;
   std::string referenceRange;
   std::string abnormalFlag;
   std::string resultStatus;
   std::string observer;
   int         binaryValue = 0;
   std::string binaryApp;
   std::string binaryType;
   std::string binaryEncoding;
   std::string binaryData;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HL7Obx, obxId, dbObrId, dbPidId,
   name, value, unit, referenceRange, abnormalFlag, resultStatus,
   observer, binaryValue, binaryApp, binaryType, binaryEncoding, binaryData)

using HL7ObxList = std::vector<HL7Obx>;


struct HL7Obr
{
   long obrId      = 0;
   long dbMshId    = 0;
   long dbPidId    = 0;
   std::string received;
   std::string msgDatetime;
   std::string obrIdentifier;
   std::string obrUniversalId;
   std::string obrUniversalName;
   std::string obrObservationDatetime;
   std::string obrSampleStatus;
   std::string medicalRecordNo;
   std::string pidPatientId;
   std::string pidIdentifier;
   std::string pidName;
   std::string pidGender;
   std::string parentResult;
   std::string resultCopiesTo;

   std::string veterinarian;
   std::string sendingDoctor;
   std::string clinicalInfo;

   std::string uploadResponseTime;
   std::string uploadStatus;

   HL7ObxList obxList;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HL7Obr, obrId, dbMshId, dbPidId, received, msgDatetime,
   obrIdentifier, obrUniversalId, obrUniversalName, obrObservationDatetime, obrSampleStatus, medicalRecordNo,
   pidPatientId, pidIdentifier, pidName, pidGender, parentResult, resultCopiesTo,
   veterinarian, sendingDoctor, clinicalInfo, uploadResponseTime, uploadStatus,
   obxList)

using HL7ObrList = std::vector<HL7Obr>;

} // namespace dto
} // namespace lis
} // namespace tbs
