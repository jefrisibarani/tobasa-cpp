#pragma once

#include <vector>
#include <tobasa/logger.h>
#include <tobasa/util.h>
#include <tobasa/format.h>
#include <tobasasql/sql_connection.h>
#include <tobasasql/sql_query.h>
#include <tobasasql/sql_log_identifier.h>
#include <tobasasql/sql_service_base.h>
#include <tobasasql/sql_query_option.h>
#include <tobasalis/lis/common.h>
#include <tobasalis/hl7/message.h>
#include "dto_hl7.h"
#include "lis_db_service_base.h"

namespace tbs {
namespace lis {
namespace svc {

class LisHL7ServiceBase : public LisServiceBase
{
public:
   LisHL7ServiceBase(const LisHL7ServiceBase &) = delete;
   LisHL7ServiceBase(LisHL7ServiceBase &&) = delete;

   LisHL7ServiceBase() {}
   virtual ~LisHL7ServiceBase() {}

   virtual bool saveLisData(const lis::MessagePtr& message, DbStoreResultPtr dbStoreResult=nullptr) = 0;

   /// Get HL7 Orders/observation requests
   /// allowed filter values are: mshid_{message header id}, eg: mshid_232
   /// if retrieveObx true, retrive all obx(results list) for the obr
   virtual dto::HL7ObrList getHL7ObrList(const std::string& instrumentType, const sql::SqlQueryOption& option={}, bool retrieveObx=false) = 0;

   virtual dto::HL7ObxList getHL7ObxList(const std::string& instrumentType, int64_t obrId, int64_t patientId=0) = 0;
};
using LisHL7ServicePtr = std::shared_ptr<LisHL7ServiceBase>;


template <typename SqlDriverType>
class LisHL7Service : public LisHL7ServiceBase
{
public:
   using SqlResult     = sql::SqlResult<SqlDriverType>;
   using SqlConnection = sql::SqlConnection<SqlDriverType>;
   using SqlQuery      = sql::SqlQuery<SqlDriverType>;
   using Helper        = typename SqlDriverType::HelperImpl;
   using SqlApplyLogId = sql::SqlApplyLogId<SqlDriverType>;

protected:
   SqlConnection& _sqlConn;

public:
   LisHL7Service(const LisHL7Service &) = delete;
   LisHL7Service(LisHL7Service &&) = delete;

   LisHL7Service() {}
   LisHL7Service(SqlConnection& conn) : _sqlConn{ conn }  {}
   virtual ~LisHL7Service() {}

   virtual bool saveLisData(const lis::MessagePtr& message, DbStoreResultPtr dbStoreResult) override
   {
      SqlApplyLogId applyLogId(_sqlConn, message->internalId());

      if (message->vendorProtocolId() == lis::MSG_HL7)
      {
         // TODO_JEFRI: BEGIN TRANSACTION
         auto hl7message = std::static_pointer_cast<hl7::Message>(message);
         if (doSaveLisData(hl7message, dbStoreResult))
         {
            // COMMIT TRANSACTION
            return true;
         }
         else
         {
            // ROLLBACK TRANSACTION
            return false;
         }
      }
      else
         throw AppException("Invalid protocol for LisHL7 DB service");

      return false;
   }

   /// Get HL7 Orders/observation requests
   /// allowed filter values are: mshid_{message header id}, eg: mshid_232
   /// if retrieveObx true, retrive all obx(results list) for the obr
   virtual dto::HL7ObrList getHL7ObrList(const std::string& instrumentType, const sql::SqlQueryOption& option, bool retrieveObx)
   {
      bool hasLimitOffset = option.limitOffsetValid();
      bool hasDateRange   = option.dateRangeValid();
      bool hasFilter      = false;
      std::string limitOffset, dataRange, filter;

      if (instrumentType == lis::DEV_TEST_HL7)
      {
         filter = " WHERE obr.universal_name = 'DevTest' ";
         hasFilter = true;
      }

      if (!option.filter.empty())
      {
         if (util::startsWith(option.filter, "mshid_") && option.filter.length() > 6)
         {
            // filter example: mshid_232
            std::string clause = "WHERE";
            if (hasFilter)
               clause = "AND";

            std::string mshId = option.filter.substr(6);
            if (util::isNumber(mshId)) {
               filter += tbsfmt::format( "{} msh.id = {} ", clause, mshId);
            }
         }
         else
            return {}; // invalid filter
      }

      if (hasDateRange )
      {
         std::string clause = "WHERE";
         if (hasFilter)
            clause = "AND";

         if (_sqlConn.backendType() == sql::BackendType::sqlite)
            dataRange = tbsfmt::format( "{} DATE(msg_datetime) >= :startDate AND DATE(msg_datetime) <= :endDate ", clause);
         else if (_sqlConn.backendType() == sql::BackendType::adodb || _sqlConn.backendType() == sql::BackendType::odbc)
            dataRange = tbsfmt::format( "{} msg_datetime >= :startDate AND msg_datetime <= :endDate ", clause);
         else if (_sqlConn.backendType() == sql::BackendType::pgsql)
            dataRange = tbsfmt::format( "{} CAST(msg_datetime AS date) >= :startDate AND CAST(msg_datetime AS date) <= :endDate ", clause);
         else if (_sqlConn.backendType() == sql::BackendType::mysql)
            dataRange = tbsfmt::format( "{} msg_datetime >= :startDate AND msg_datetime <= :endDate ", clause);
         else
            throw AppException("Could not apply date filtering. Unsupported database backend");
      }

      if (hasLimitOffset )
      {
         limitOffset = " ORDER BY id DESC ";
         limitOffset += Helper::limitAndOffset(option.limit, option.offset, _sqlConn.dbmsName());
      }

      // Specific table column
      std::string fieldMedicalRecordNo = "";
      std::string fiedSampleStatus = "";
      if (instrumentType == lis::DEV_TEST_HL7)
      {
         fiedSampleStatus = "obr.placer_field1 AS sample_status";
         fieldMedicalRecordNo = "pid.identifier AS medical_record_no";
      }
      else
      {
         fiedSampleStatus     = "'' AS sample_status";
         fieldMedicalRecordNo = "'' AS medical_record_no";
      }

      std::string sql =
         tbsfmt::format(R"-(
            SELECT obr.id,
                  obr.db_message_id, obr.db_patient_id, msh.received, msh.msg_datetime, obr.identifier,
                  obr.universal_id, obr.universal_name, obr.observation_datetime, obr.veterinarian,
                  obr.sending_doctor, obr.clinical_info, {}, {},
                  pid.patient_id AS pid_patient_id, pid.identifier AS pid_identifier, pid.name   AS pid_name,
                  pid.gender AS pid_gender, obr.parent_result, obr.result_copies_to,
                  up.response_time AS upload_response_time, up.status AS upload_status
            FROM lis_hl7_observation_requests obr
            JOIN lis_hl7_headers msh ON obr.db_message_id = msh.id
            JOIN lis_hl7_patients pid ON obr.db_patient_id = pid.id
            LEFT JOIN lis_message_http_upload up ON up.id = (SELECT MAX(u.id) FROM lis_message_http_upload u WHERE u.header_id = msh.id) 
            {}  {}  {}  )-" , fiedSampleStatus, fieldMedicalRecordNo, filter, dataRange , limitOffset);

      SqlQuery query(_sqlConn, sql);
      if (hasDateRange)
      {
         query.addParam("startDate", sql::DataType::timestamp, option.startDate);
         query.addParam("endDate",   sql::DataType::timestamp, option.endDate);
      }

      auto sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         dto::HL7ObrList list;
         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);
            list.emplace_back( getHL7Obr(instrumentType, sqlResult, retrieveObx) );
         }

         return list;
      }

      return {};
   }

   virtual dto::HL7ObxList getHL7ObxList(const std::string& instrumentType, int64_t obrId, int64_t patientId)
   {
      std::string sql = 
         R"-( SELECT obx.id,
                  obx.db_message_id, obx.db_patient_id, obx.db_obr_id, obx.identifier_name,
                  obx.result_value, obx.result_unit, obx.references_range, obx.abnormal_flags,
                  obx.result_status, obx.responsible_observer, obx.binary_value,
                  obx.binary_app, obx.binary_type, obx.binary_encoding, obx.binary_data
              FROM lis_hl7_observation_results obx
              WHERE obx.db_obr_id = :obrId )-";

      if (patientId > 0) {
         sql += " AND obx.db_patient_id = :patId";
      }

      SqlQuery query(_sqlConn, sql);
      query.addParam("obrId", sql::DataType::bigint, obrId);

      if (patientId > 0) {
         query.addParam("patId", sql::DataType::bigint, patientId);
      }

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();

      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         dto::HL7ObxList list;
         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);
            list.emplace_back( getHL7Obx(instrumentType, sqlResult) );
         }

         return list;
      }

      return {};
   }

protected:

   virtual std::string dbVersionString() override
   {
      return _sqlConn.databaseName() + " " + _sqlConn.versionString();
   }

   virtual dto::HL7Obx getHL7Obx(const std::string& instrumentType, std::shared_ptr<SqlResult> sqlResult)
   {
      dto::HL7Obx obx;

      obx.obxId            = sqlResult->getLongValue("id");
      obx.dbObrId          = sqlResult->getLongValue("db_obr_id");
      obx.dbPidId          = sqlResult->getLongValue("db_patient_id");
      obx.name             = sqlResult->getStringValue("identifier_name");
      obx.value            = sqlResult->getStringValue("result_value");
      obx.unit             = sqlResult->getStringValue("result_unit");
      obx.referenceRange   = sqlResult->getStringValue("references_range");
      obx.abnormalFlag     = sqlResult->getStringValue("abnormal_flags");
      obx.resultStatus     = sqlResult->getStringValue("result_status");

      obx.observer         = sqlResult->getStringValue("responsible_observer");
      obx.binaryValue      = sqlResult->getLongValue("binary_value");
      obx.binaryApp        = sqlResult->getStringValue("binary_app");
      obx.binaryType       = sqlResult->getStringValue("binary_type");
      obx.binaryEncoding   = sqlResult->getStringValue("binary_encoding");
      obx.binaryData       = sqlResult->getStringValue("binary_data");

      if (obx.abnormalFlag == "H")
         obx.abnormalFlag = "High";
      if (obx.abnormalFlag == "L")
         obx.abnormalFlag = "Low";
      if (obx.abnormalFlag == "N")
         obx.abnormalFlag = "Normal";

      if (obx.resultStatus == "F")
         obx.resultStatus = "Final";

      return std::move(obx);
   }

   virtual dto::HL7Obr getHL7Obr(const std::string& instrumentType, std::shared_ptr<SqlResult> sqlResult, bool retrieveObx)
   {
      dto::HL7Obr obr;

      obr.obrId                  = sqlResult->getLongValue("id");
      obr.dbMshId                = sqlResult->getLongValue("db_message_id");
      obr.dbPidId                = sqlResult->getLongValue("db_patient_id");
      obr.received               = sqlResult->getStringValue("received");
      obr.msgDatetime            = sqlResult->getStringValue("msg_datetime");
      obr.obrIdentifier          = sqlResult->getStringValue("identifier");
      obr.obrUniversalId         = sqlResult->getStringValue("universal_id");
      obr.obrUniversalName       = sqlResult->getStringValue("universal_name");
      obr.obrObservationDatetime = sqlResult->getStringValue("observation_datetime");
      obr.obrSampleStatus        = sqlResult->getStringValue("sample_status");
      obr.medicalRecordNo        = sqlResult->getStringValue("medical_record_no");
      obr.pidPatientId           = sqlResult->getStringValue("pid_patient_id");
      obr.pidIdentifier          = sqlResult->getStringValue("pid_identifier");
      obr.pidName                = sqlResult->getStringValue("pid_name");
      obr.pidGender              = sqlResult->getStringValue("pid_gender");

      obr.parentResult           = sqlResult->getStringValue("parent_result");
      obr.resultCopiesTo         = sqlResult->getStringValue("result_copies_to");

      obr.uploadResponseTime     = sqlResult->getStringValue("upload_response_time");
      obr.uploadStatus           = sqlResult->getStringValue("upload_status");

      obr.veterinarian          = sqlResult->getStringValue("veterinarian");
      obr.sendingDoctor         = sqlResult->getStringValue("sending_doctor");
      obr.clinicalInfo          = sqlResult->getStringValue("clinical_info");

      obr.obxList                = {};

      if (obr.uploadStatus.empty()) {
         obr.uploadStatus = "NONE";
      }

      if (obr.pidGender == "F")
         obr.pidGender = "Female";
      if (obr.pidGender == "M")
         obr.pidGender = "Male";
      if (obr.pidGender == "O")
         obr.pidGender = "Other";

      if (retrieveObx) {
         obr.obxList = getHL7ObxList(instrumentType, obr.obrId, obr.dbPidId);
      }

      return std::move(obr);
   }

   virtual bool doSaveLisData(const hl7::MessagePtr& message, DbStoreResultPtr dbStoreResult)
   {
      try
      {
         if ( message->segments("PID").size() > 1 ) {
            Logger::logW("[lis_db] [msg:{}] HL7 message contains more than one PID", message->internalId());
         }

         if ( message->segments("OBR").size() > 1 ) {
            Logger::logW("[lis_db] [msg:{}] HL7 message contains more than one OBR", message->internalId());
         }

         std::string instrumentType     = message->instrumentType();
         std::string sendingApplication = message->getValue("MSH.3");
         std::string sendingFacility    = message->getValue("MSH.4");

         int sequence                   = 0;
         int patientIndex               = 0;
         int patientVisitIndex          = 0;
         int obrIndex                   = 0;
         int obxIndex                   = 0;

         int64_t dbMessageId            = 0;
         int64_t dbPatientId            = 0;
         int64_t dbPatientVisitId       = 0;
         int64_t dbObrId                = 0;

         for (auto segment: message->segments())
         {
            sequence = segment->sequenceNo();

            if (segment->name() == "MSH" )
            {
               if (sequence != 0)
               {
                  Logger::logE("[lis_db] [msg:{}] Error occured saving HL7 message. Invalid MSH sequence no: {}", message->internalId(), sequence);
                  return false;
               }

               dbMessageId = saveLisDataHeader(message);
            }
            else
            {
               if (dbMessageId == 0)
               {
                  Logger::logE("[lis_db] [msg:{}] Error occured saving HL7 message. Invalid MSH dbMessageId: {}", message->internalId(), dbMessageId);
                  return false;
               }

               if (segment->name() == "PID" )
               {
                  dbPatientId = saveLisDataPatient(patientIndex, message, dbMessageId);
                  patientIndex++;
               }
               else if (segment->name() == "PV1" )
               {
                  if (dbPatientId == 0)
                  {
                     Logger::logE("[lis_db] [msg:{}] Error occured saving HL7 message. Invalid MSH dbPatientId: {}", message->internalId(), dbPatientId);
                     return false;
                  }

                  auto dbPatientVisitId = saveLisDataPatientVisit(patientVisitIndex, message, dbMessageId, dbPatientId);
                  patientVisitIndex++;
               }
               else if (segment->name() == "OBR")
               {
                  if (dbPatientId == 0)
                  {
                     Logger::logE("[lis_db] [msg:{}] Error occured saving HL7 message. Invalid MSH dbPatientId: {}", message->internalId(), dbPatientId);
                     return false;
                  }

                  dbObrId = saveLisDataObservationRequest(obrIndex, message, dbMessageId, dbPatientId);
                  obrIndex++;
               }
               else if (segment->name() == "OBX")
               {
                  if (dbObrId == 0)
                  {
                     Logger::logE("[lis_db] [msg:{}] Error occured saving HL7 message. Invalid MSH dbObrId: {}", message->internalId(), dbObrId);
                     return false;
                  }

                  saveLisDataObservationResult(obxIndex, message, dbMessageId, dbPatientId, dbObrId);
                  obxIndex ++;
               }
               else {
                  Logger::logW("[lis_db] [msg:{}] Saving HL7 data, found unknown segment {}", message->internalId(), segment->name());
               }
            }
         }

         dbStoreResult->headerId = dbMessageId;
         dbStoreResult->internalMessageId = message->internalId();

         return true;
      }
      catch (const std::exception& ex)
      {
         Logger::logE("[lis_db] [msg:{}] Error occured saving HL7 data: {}", message->internalId(), ex.what());
      }

      return false;
   }

   virtual int64_t saveLisDataHeader(const hl7::MessagePtr& message)
   {
      return 0;
   }

   virtual int64_t saveLisDataPatient(int index, const hl7::MessagePtr& message, int64_t dbMessageId)
   {
      return 0;
   }

   virtual int64_t saveLisDataPatientVisit(int index, const hl7::MessagePtr& message, int64_t dbMessageId, int64_t dbPatientId)
   {
      return 0;
   }

   virtual int64_t saveLisDataPatient(int index, const hl7::MessagePtr& message, int64_t dbMessageId, int64_t dbPatientId)
   {
      return 0;
   }

   virtual int64_t saveLisDataObservationRequest(int index, const hl7::MessagePtr& message, int64_t dbMessageId, int64_t dbPatientId)
   {
      return 0;
   }

   virtual bool    saveLisDataObservationResult(int index, const hl7::MessagePtr& message, int64_t dbMessageId, int64_t dbPatientId, int64_t dbObrId)
   {
      return true;
   }
};

} // namespace svc
} // namespace lis
} // namespace tbs


