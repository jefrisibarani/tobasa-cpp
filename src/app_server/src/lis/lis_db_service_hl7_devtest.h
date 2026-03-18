#pragma once

#include "lis_db_service_hl7.h"

namespace tbs {
namespace lis {
namespace svc {

template < typename SqlDriverType >
class LisHL7ServiceDevTest : public LisHL7Service<SqlDriverType>
{
public:
   using SqlResult     = sql::SqlResult<SqlDriverType>;
   using SqlConnection = sql::SqlConnection<SqlDriverType>;
   using SqlQuery      = sql::SqlQuery<SqlDriverType>;
   using Helper        = typename SqlDriverType::HelperImpl;
   using SqlApplyLogId = sql::SqlApplyLogId<SqlDriverType>;

public:
   LisHL7ServiceDevTest() {}
   LisHL7ServiceDevTest(SqlConnection& conn) 
      : LisHL7Service<SqlDriverType>{ conn } {}
      
   virtual ~LisHL7ServiceDevTest() {}

protected:

   /// MSH - Message Header - Dev Test HL7
   virtual int64_t saveLisDataHeader(const hl7::MessagePtr& message) override
   {
      // MSH-3 Sending Application
      std::string sendingApp      = message->getValue("MSH.3");
      // MSH-4 Sending Facility, Use Dev Test HL7
      std::string sendingFacility = message->getValue("MSH.4");
      // MSH-7 Data/Time of current Message, Call the system time message
      std::string msgDatetime     = message->getValue("MSH.7");
      // MSH-9 Message type, e.g.:ORUAR01
      std::string msgType         = message->getValue("MSH.9");
      // MSH-10 Message Control ID， Uniquely identifies a Message, It increase by 1 along with the increasing of Message number.
      std::string msgControlId    = message->getValue("MSH.10");
      // MSH-11 Processing ID, set as P.
      std::string processingId    = message->getValue("MSH.11");
      // MSH-12 Version ID, HL7 Version is 2.3.1
      std::string versionId       = message->getValue("MSH.12");
      // MSH-16 Application response type, the type of results as the sender.
      //        0 - patient sample test results; 1 -Calibration Results 2 - quality control results
      std::string appAckType      = message->getValue("MSH.16");
      // MSH-18 Character set, using ASCII code.
      std::string characterSet    = message->getValue("MSH.18");
      std::string rawdata         = message->segments("MSH")[0]->value();

      std::string outputClauseMSSQL;
      if (this->_sqlConn.backendType() == sql::BackendType::adodb || this->_sqlConn.backendType() == sql::BackendType::odbc) {
         outputClauseMSSQL = " OUTPUT inserted.ID ";
      }

      std::string sql = tbsfmt::format(R"-(
               INSERT INTO lis_hl7_headers
                  ( received,        sending_app,  sending_facility,
                    msg_datetime,    msg_type,     msg_control_id,
                    processing_id,   version_id,   app_ack_type,
                    character_set,   rawdata ) {}
               VALUES ( :rcvd, :sapp, :faci, :mdtime, :mtype, :mcid, :prid, :vid, :ackTyp, :cset, :raw ) )-", outputClauseMSSQL );

      if (this->_sqlConn.backendType() == sql::BackendType::pgsql || this->_sqlConn.backendType() == sql::BackendType::sqlite) {
         sql += " RETURNING id ";
      }

      using dtype = sql::DataType;
      SqlQuery query(this->_sqlConn, sql);
      query.addParam("rcvd",   dtype::timestamp, DateTime::now().isoDateTimeString());
      query.addParam("sapp",   dtype::varchar, sendingApp);
      query.addParam("faci",   dtype::varchar, sendingFacility);
      query.addParam("mdtime", dtype::varchar, this->getDateTime(msgDatetime));
      query.addParam("mtype",  dtype::varchar, msgType);
      query.addParam("mcid",   dtype::varchar, msgControlId);
      query.addParam("prid",   dtype::varchar, processingId);
      query.addParam("vid",    dtype::varchar, versionId);
      query.addParam("ackTyp", dtype::varchar, appAckType);
      query.addParam("cset",   dtype::varchar, characterSet);
      query.addParam("raw",    dtype::varchar, rawdata);

      this->throwIfNotSupportedBackend(this->_sqlConn.backendType());
      int64_t dbMessageId = -1;
      if (this->_sqlConn.backendType() == sql::BackendType::mysql)
      {
         int affected = query.execute();
         if (affected > 0)
            dbMessageId = this->_sqlConn.lastInsertRowid();
      }
      else
      {
         auto newId = query.executeScalar();
         if (util::isNumber(newId)) 
            dbMessageId = std::stoll(newId);
         else
            throw AppException("Could not insert message header data into lis_hl7_headers" );
      }

      if (dbMessageId > 0)
         return dbMessageId;
      else
         throw AppException("Invalid ID for lis_hl7_headers" );
   }

   /// PID - Patient identification segment - Dev Test HL7
   virtual int64_t saveLisDataPatient(int index, const hl7::MessagePtr& message, int64_t dbMessageId) override
   {
      auto segment = message->segments("PID")[index];

      // PID-1 Set ID - PID : Message segment identified for different patients
      std::string setId         = segment->fields(1)->value();
      // PID-2 The patient's hospital number
      std::string patientId     = segment->fields(2)->value();
      // PID-4 Alternate Patient ID-PID : Used as Bed No.
      std::string altPatientId  = segment->fields(4)->value();
      // PID-5 Patient Name
      std::string name          = segment->fields(5)->value();
      // PID-6 Mother’s Maiden Name: used as Ward
      std::string motherName    = segment->fields(6)->value();
      // PID-8 Gender Male, send M; Female, Send F; other send O
      std::string gender        = segment->fields(8)->value();

      // PID-26 Citizenship : Note: sample identification information, such as: jaundice, chyle, hemolysis
      std::string citizenship   = segment->fields(26)->value();
      std::string rawdata       = segment->value();

      std::string outputClauseMSSQL;
      if (this->_sqlConn.backendType() == sql::BackendType::adodb || this->_sqlConn.backendType() == sql::BackendType::odbc) {
         outputClauseMSSQL = " OUTPUT inserted.ID ";
      }

      std::string sql = tbsfmt::format(R"-(
               INSERT INTO lis_hl7_patients
                  (  db_message_id, set_id,      patient_id, identifier,
                     name,          mother_name, dob,        gender,
                     field_4,       field_26,    rawdata ) {}
               VALUES ( :mid, :sid, :pid, :ident, :name, :mname, :dob, :gender, :fi4, :fi26, :raw ) )-", outputClauseMSSQL );

      if (this->_sqlConn.backendType() == sql::BackendType::pgsql || this->_sqlConn.backendType() == sql::BackendType::sqlite) {
         sql += " RETURNING id ";
      }

      using dtype = sql::DataType;
      SqlQuery query(this->_sqlConn, sql);
      query.addParam("mid",    dtype::bigint,  dbMessageId);
      query.addParam("sid",    dtype::varchar, setId);
      query.addParam("pid",    dtype::varchar, patientId);
      query.addParam("ident",  dtype::varchar, patientId); // we use patientId as identifier
      query.addParam("name",   dtype::varchar, name);
      query.addParam("mname",  dtype::varchar, motherName);
      query.addParam("dob",    dtype::varchar, std::string() );
      query.addParam("gender", dtype::varchar, gender);
      query.addParam("fi4",    dtype::varchar, altPatientId);
      query.addParam("fi26",   dtype::varchar, citizenship);
      query.addParam("raw",    dtype::varchar, rawdata);

      this->throwIfNotSupportedBackend(this->_sqlConn.backendType());
      int64_t dbPatientId = -1;
      if (this->_sqlConn.backendType() == sql::BackendType::mysql)
      {
         int affected = query.execute();
         if (affected > 0)
            dbPatientId = this->_sqlConn.lastInsertRowid();
      }
      else
      {
         auto newId = query.executeScalar();
         if (util::isNumber(newId)) 
            dbPatientId = std::stoll(newId);
         else
            throw AppException("Could not insert patient data into lis_hl7_patients" );
      }

      if (dbPatientId > 0)
         return dbPatientId;
      else
         throw AppException("Invalid ID for lis_hl7_patients" );
   }

   /// OBR (Observation Request)  - test report information - Dev Test HL7
   virtual int64_t saveLisDataObservationRequest(int index, const hl7::MessagePtr& message, int64_t dbMessageId, int64_t dbPatientId) override
   {
      auto segment = message->segments("OBR")[index];

      // OBR-1 Set ID — OBR :dentify different OBR segments
      std::string setId               = segment->fields(1)->value(); // message->getValue("OBR.1");
      // OBR-3 Filler Order Number : Sample Serial No. / machine user interface use this to fill in medical record number
      std::string identifier          = segment->fields(3)->value();

      // OBR-4 Universal Service ID : Universal Service ID, Brand^Type
      std::string universalId         = segment->fields(4)->components()[0]->value(); // message->getValue("OBR.4.1");
      std::string universalName       = segment->fields(4)->components()[1]->value(); // message->getValue("OBR.4.2");

      // OBR-5 Priority (Stat Sample)   : Stat status，Y means Yes，N means N
      std::string priorityObr         = segment->fields(5)->value();
      // OBR-7 Observation Date/Time,                                e.g: 20240523
      std::string observationDatetime = segment->fields(7)->value();
      // OBR-14 Specimen Received Date/Time : Inspection time,       e.g 20240523
      std::string specimenDatetime    = segment->fields(14)->value();
      // OBR-16 Ordering Provider       : Sending doctor
      std::string sendingDoctor       = segment->fields(16)->value();
      // OBR-17 Order Callback Phone Number : Sending department
      std::string sendingDepartment   = segment->fields(17)->value();
      // OBR-18 Placer Field 1          : Sample status, such as, icterus 、hemolysis、lipemia
      std::string placerField1        = segment->fields(18)->value();
      std::string rawdata             = segment->value();

      std::string outputClauseMSSQL;
      if (this->_sqlConn.backendType() == sql::BackendType::adodb || this->_sqlConn.backendType() == sql::BackendType::odbc) {
         outputClauseMSSQL = " OUTPUT inserted.ID ";
      }

      std::string sql = tbsfmt::format(R"-(
         INSERT INTO lis_hl7_observation_requests
            (  db_message_id, db_patient_id, set_id, identifier, universal_id,
               universal_name, universal_encodesys, priority_obr, requested_datetime,
               observation_datetime, veterinarian, clinical_info, speciment_datetime,
               sending_doctor, sending_department, placer_field1, diag_maker_id,
               parent_result, result_copies_to, principal_result, rawdata ) {}
         VALUES ( :dbmid, :dbpid, :sid, :ident, :unid, :unname, :unenc, :pobr, :reqDt, :obDt, :vet,
                  :cinfo, :specDt, :sdoctor, :sdept, :pfld1, :diag, :parResult, :resCopy, :princ, :raw )
         )-", outputClauseMSSQL );

      if (this->_sqlConn.backendType() == sql::BackendType::pgsql || this->_sqlConn.backendType() == sql::BackendType::sqlite) {
         sql += " RETURNING id ";
      }

      using dtype = sql::DataType;
      SqlQuery query(this->_sqlConn, sql);
      query.addParam("dbmid",     dtype::bigint,  dbMessageId);
      query.addParam("dbpid",     dtype::bigint,  dbPatientId);
      query.addParam("sid",       dtype::varchar, setId);
      query.addParam("ident",     dtype::varchar, identifier);
      query.addParam("unid",      dtype::varchar, universalId);
      query.addParam("unname",    dtype::varchar, universalName);
      query.addParam("unenc",     dtype::varchar, std::string() );
      query.addParam("pobr",      dtype::varchar, priorityObr);
      query.addParam("reqDt",     dtype::varchar, std::string() );
      query.addParam("obDt",      dtype::varchar, this->getDateTime(observationDatetime));
      query.addParam("vet",       dtype::varchar, std::string() );
      query.addParam("cinfo",     dtype::varchar, std::string() );
      query.addParam("specDt",    dtype::varchar, this->getDateTime(specimenDatetime));
      query.addParam("sdoctor",   dtype::varchar, sendingDoctor);
      query.addParam("sdept",     dtype::varchar, sendingDepartment);
      query.addParam("pfld1",     dtype::varchar, placerField1);
      query.addParam("diag",      dtype::varchar, std::string() );
      query.addParam("parResult", dtype::varchar, std::string() );
      query.addParam("resCopy",   dtype::varchar, std::string() );
      query.addParam("princ",     dtype::varchar, std::string() );
      query.addParam("raw",       dtype::varchar, rawdata);

      this->throwIfNotSupportedBackend(this->_sqlConn.backendType());
      int64_t dbObservationRequestId = -1;
      if (this->_sqlConn.backendType() == sql::BackendType::mysql)
      {
         int affected = query.execute();
         if (affected > 0)
            dbObservationRequestId = this->_sqlConn.lastInsertRowid();
      }
      else
      {
         auto newId = query.executeScalar();
         if (util::isNumber(newId)) 
            dbObservationRequestId = std::stoll(newId);
         else
            throw AppException("Could not insert message header data into lis_hl7_observation_requests" );
      }

      if (dbObservationRequestId <= 0)
         throw AppException("Invalid ID for lis_hl7_observation_requests" );      
      
      return dbObservationRequestId;
   }

   /// OBX (Observation/Result) - parameter information of each test result - Dev Test HL7
   virtual bool saveLisDataObservationResult(int index, const hl7::MessagePtr& message, int64_t dbMessageId, int64_t dbPatientId, int64_t dbObrId) override
   {
      auto obx = message->segments("OBX")[index];

      // OBX-1 Set ID-OBX
      std::string setId               = obx->fields(1)->value();
      // OBX-2 Value Type : Use to identify the type of testing results.
      //       NM- means Numeric, for quantitative Items. ST- means Character String, for qualitative items.
      std::string valueType           = obx->fields(2)->value();
      // OBX-3 Observation Identifier : Used as item ID number
      std::string identifierId        = obx->fields(3)->value();
      // OBX-4 Observation Sub-ID: Used as item Name
      std::string identifierName      = obx->fields(4)->value();
      // OBX-5 Observation Value
      std::string resultValue         = obx->fields(5)->value();
      // OBX-6 Units
      std::string resultUnit          = obx->fields(6)->value();
      // OBX-7 Reference Range
      std::string referencesRange     = obx->fields(7)->value();
      // OBX-8 Abnormal Flags, L: means Result is Low, H:results is high, N: Resutls are normal
      std::string abnormalFlags       = obx->fields(8)->value();
      // OBX-11 Status of the analysis result: F- Means Final Results
      std::string resultStatus        = obx->fields(11)->value();
      // OBX-13 User defined access check
      std::string userDefinedCheck    = obx->fields(13)->value();
      // OBX-14 Data/time of the observation: Used as testing data, e.g 20240523
      std::string observationDatetime = obx->fields(14)->value();
      // OBX-16 Responsible Observer: Used as Doctor
      std::string responsibleObserver = obx->fields(16)->value();
      std::string rawdata             = obx->value();

      std::string outputClauseMSSQL;
      if (this->_sqlConn.backendType() == sql::BackendType::adodb || this->_sqlConn.backendType() == sql::BackendType::odbc) {
         outputClauseMSSQL = " OUTPUT inserted.ID ";
      }

      std::string sql = tbsfmt::format(R"-(
               INSERT INTO lis_hl7_observation_results
                  (  db_message_id, db_patient_id, db_obr_id, set_id, value_type,
                     identifier_id, identifier_name, identifier_encode_sys, result_value, result_unit,
                     references_range, abnormal_flags, result_status, user_defined_checks,
                     observation_datetime, responsible_observer, binary_value,
                     binary_app, binary_type, binary_encoding, binary_data, rawdata ) {}
               VALUES ( :dbmid, :dbpid, :dbobrid, :setid, :valType, :identId, :identName, :identEnc, :resVal, :resUnit,
                        :refRange, :abFlags, :resStatus, :udefCheck, :obsDt, :respObs, :binVal, :binApp, :binType, :binEnc, :binData, :raw )
            )-", outputClauseMSSQL );

      if (this->_sqlConn.backendType() == sql::BackendType::pgsql || this->_sqlConn.backendType() == sql::BackendType::sqlite) {
         sql += " RETURNING id ";
      }

      using dtype = sql::DataType;
      SqlQuery query(this->_sqlConn, sql);

      query.addParam("dbmid",     dtype::bigint,  dbMessageId);
      query.addParam("dbpid",     dtype::bigint,  dbPatientId);
      query.addParam("dbobrid",   dtype::bigint,  dbObrId);
      query.addParam("setid",     dtype::varchar, setId);
      query.addParam("valType",   dtype::varchar, valueType);

      query.addParam("identId",   dtype::varchar, identifierId);
      query.addParam("identName", dtype::varchar, identifierName);
      query.addParam("identEnc",  dtype::varchar, std::string() );
      query.addParam("resVal",    dtype::varchar, resultValue);
      query.addParam("resUnit",   dtype::varchar, resultUnit);

      query.addParam("refRange",  dtype::varchar, referencesRange);
      query.addParam("abFlags",   dtype::varchar, abnormalFlags);
      query.addParam("resStatus", dtype::varchar, resultStatus);
      query.addParam("udefCheck", dtype::varchar, userDefinedCheck);
      query.addParam("obsDt",     dtype::varchar, this->getDateTime(observationDatetime));
      query.addParam("respObs",   dtype::varchar, responsibleObserver);

      query.addParam("binVal",    dtype::tinyint, (int) 0 );
      query.addParam("binApp",    dtype::varchar, std::string() );
      query.addParam("binType",   dtype::varchar, std::string() );
      query.addParam("binEnc",    dtype::varchar, std::string() );
      query.addParam("binData",   dtype::varchar, std::string() );
      query.addParam("raw",       dtype::varchar, rawdata);

      this->throwIfNotSupportedBackend(this->_sqlConn.backendType());
      int64_t dbObservationResultId = -1;
      if (this->_sqlConn.backendType() == sql::BackendType::mysql)
      {
         int affected = query.execute();
         if (affected > 0)
            dbObservationResultId = this->_sqlConn.lastInsertRowid();
      }
      else
      {
         auto newId = query.executeScalar();
         if (util::isNumber(newId)) 
            dbObservationResultId = std::stoll(newId);
         else
            throw AppException("Could not insert message header data into lis_hl7_observation_results" );
      }

      if (dbObservationResultId <= 0)
         throw AppException("Invalid ID for lis_hl7_observation_results" );
      
      return dbObservationResultId;
   }
};

} // namespace svc
} // namespace lis
} // namespace tbs


