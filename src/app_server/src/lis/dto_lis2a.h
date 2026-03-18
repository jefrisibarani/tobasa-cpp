#pragma once

#include <string>
#include <vector>
#include <tobasa/json.h>

namespace tbs {
namespace lis {
namespace dto {

// Result
struct LIS2AResult
{
   long headerId  = 0;
   long patientId = 0;
   long orderId   = 0;
   long resultId  = 0;
   std::string msgDatetime;
   std::string patientName;
   std::string patientLastname;
   std::string specId;
   std::string specType;
   std::string instrument;
   std::string testCode;
   std::string value;
   std::string flag;
   std::string unit;
   std::string range;
   std::string abnormalFlag;
   std::string status;
   int         binaryValue = 0;
   std::string binaryApp;
   std::string binaryType;
   std::string binaryEncoding;
   std::string binaryData;

};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LIS2AResult, headerId, patientId, orderId, resultId,
   msgDatetime, patientName, patientLastname, specId, specType, instrument,
   testCode, value, flag, unit, range, abnormalFlag, status,
   binaryValue, binaryApp, binaryType, binaryEncoding, binaryData)

using LIS2AResultList = std::vector<LIS2AResult>;


// Order
struct LIS2AEntry
{
   long headerId = 0;
   std::string msgDatetime;
   std::string patientName;
   std::string specId;
   std::string specType;
   std::string instrument;

   std::string veterinarian;
   std::string sendingDoctor;
   std::string clinicalInfo;

   std::string uploadResponseTime;
   std::string uploadStatus;

   LIS2AResultList resultList;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LIS2AEntry, headerId, msgDatetime, patientName, specId, specType, instrument,
   veterinarian, sendingDoctor, clinicalInfo, uploadResponseTime, uploadStatus, 
   resultList)

using LIS2AEntryList = std::vector<LIS2AEntry>;

} // namespace dto
} // namespace lis
} // namespace tbs  
