#pragma once

#include <tobasasql/sql_connection.h>
#include <tobasasql/sql_query.h>
#include <tobasasql/sql_log_identifier.h>
#include "tobasalis/lis2a/subrecord.h"
#include "tobasalis/lis2a/order.h"
#include "tobasalis/lis2a/requestinfo.h"
#include "lis_db_service_lis2a.h"

namespace tbs {
namespace lis {
namespace svc {

template < typename SqlDriverType >
class Lis2aServiceDevTest : public Lis2aService<SqlDriverType> 
{
public:
   using SqlResult     = sql::SqlResult<SqlDriverType>;
   using SqlConnection = sql::SqlConnection<SqlDriverType>;
   using SqlQuery      = sql::SqlQuery<SqlDriverType>;
   using Helper        = typename SqlDriverType::HelperImpl;
   using SqlApplyLogId = sql::SqlApplyLogId<SqlDriverType>;

public:
   Lis2aServiceDevTest() {}
   Lis2aServiceDevTest(SqlConnection& conn) 
      : Lis2aService<SqlDriverType>{ conn }  {}

   virtual ~Lis2aServiceDevTest() {}

protected:
   virtual int64_t saveLisDataOrder(const lis2a::RecordPtr& record, 
      const std::string& instrumentType, 
      int64_t dbPatientId, std::string& outSpecId, 
      std::string& outInstSpecId) override
   {
      auto rec = std::static_pointer_cast<lis2a::OrderRecord>(record);
      if (!rec)
         throw AppException("Invalid OrderRecord");

      int         sequenceNo = rec->sequenceNumber();
      auto        specIdArr  = util::split(rec->specimenID(), rec->componentDelimiter());
      std::string specId0    = specIdArr.size() > 0 ? specIdArr[0] : "";
      std::string specId1    = specIdArr.size() > 1 ? specIdArr[1] : "";
      std::string specId2    = specIdArr.size() > 2 ? specIdArr[2] : "";
      std::string specId3    = specIdArr.size() > 3 ? specIdArr[3] : "";

      auto        insSpecArr = util::split(rec->analyzerSpecimenID(), rec->componentDelimiter());
      std::string insSpecId0 = insSpecArr.size() > 0 ? insSpecArr[0] : "";             // instrument seq number
      std::string insSpecId1 = insSpecArr.size() > 1 ? insSpecArr[1] : "";             // sampling method

      lis2a::UniversalTestID testID = rec->testID();
      std::string testCode = testID.manufacturerCode();

      std::string specCollDatetime = this->getDateTime(rec->specimenCollectionDateTime());

      std::string specDesc      = rec->specimenDescriptor();                           // field 16
      auto        specDescdArr  = util::split(specDesc, rec->componentDelimiter());
      std::string specTypeTmp   = specDescdArr.size() > 0 ? specDescdArr[0] : "";      // specimen type
      std::string specType      = this->translateSampleDescriptor(specTypeTmp);
      std::string specSource    = specDescdArr.size() > 1 ? specDescdArr[1] : "";      // specimen source

      std::string physician     = rec->orderingPhysician();
      auto        physicianArr  = util::split(physician, rec->componentDelimiter());
      std::string physicianName = physicianArr.size() > 0 ? physicianArr[0] : "";

      std::string instrument    = instrumentType;
      std::string rawdata       = rec->toString();

      // -------------------------------------------------------
      // D specifics

      std::string testGroup          = testID.optionalField1(); ; 
      std::string priorityId         = getPriority(rec->priorityEnum() ) ; 
      std::string orderedDateTime    = this->getDateTime(rec->orderedDateTime());
      std::string actionCode         = getActionCode(rec->actionCodeEnum()) ; 

      std::string userField1         = rec->userField1();                                          // field 19
      auto        userField1Arr      = util::split(rec->userField1(), rec->componentDelimiter());
      std::string qcId               = userField1Arr.size() > 0 ? userField1Arr[0] : "";           // qc_id
      std::string qcExpDateTime      = userField1Arr.size() > 1 ? this->getDateTime(userField1Arr[1]) : "1900-01-01 00:00:00";  // qc_exp_datetime
      std::string qcLotNumber        = userField1Arr.size() > 2 ? userField1Arr[2] : "";           // qc_lot_number

      std::string labField1          = rec->laboratoryField1();                                    // field 21
      auto        labField1Arr       = util::split(labField1, rec->componentDelimiter());
      std::string orderingFacility   = labField1Arr.size() > 0 ? labField1Arr[0] : "";             // ordering_facility
      std::string specDilutionFactor = labField1Arr.size() > 1 ? labField1Arr[1] : "";             // spec_dilution_factor
      std::string autoRerunAllowed   = labField1Arr.size() > 2 ? labField1Arr[2] : "";             // auto_rerun_allowed
      std::string autoReflexAllowed  = labField1Arr.size() > 3 ? labField1Arr[3] : "";             // auto_reflex_allowed
      std::string parentSpecId       = rec->laboratoryField2();
      std::string resultReported     = this->getDateTime(rec->lastModified());
      std::string reportType         = getOrderReportType(rec->reportTypeEnum() );

      // -------------------------------------------------------

      std::string outputClauseMSSQL;
      if (this->_sqlConn.backendType() == sql::BackendType::adodb || this->_sqlConn.backendType() == sql::BackendType::odbc) {
         outputClauseMSSQL = " OUTPUT INSERTED.id, INSERTED.spec_id, INSERTED.inst_spec_id ";
      }

      std::string sql = tbsfmt::format(R"-(
         INSERT INTO lis_orders ( 
                  patient_id,           seq_no,             spec_id,           spec_id1
               , spec_id2,             spec_id3,           inst_spec_id,      inst_spec_id1
               , test_code,            spec_coll_datetime, spec_type,         spec_source
               , physician_name,       instrument,         raw_data 
               , test_group,           priority_id,        ordered_datetime,  action_code     
               , qc_id,                qc_exp_datetime,    qc_lot_number,     ordering_facility
               , spec_dilution_factor, auto_rerun_allowed, auto_reflex_allowed
               , parent_spec_id,       result_reported,    report_type ) {}
         VALUES ( :patId, :seqNno, :specid0, :specid1, :specid2 :specid3, :instSpecId, :instSpecId1, :testCode, :specCollDt, :specType
                , :specSource, :physicianName, :instrument, :rawdata, :testGroup, :priorityId, :orderedDt, :actionCode, :qcid, :qcExpDt 
                , :qcLotNumber, :odrFacility, :specDiluFac, :rerunAllowed, :reflexAllowed, :parentSpecId, :resultReported, :reportType )
         )-", outputClauseMSSQL );

      if (this->_sqlConn.backendType() == sql::BackendType::pgsql || this->_sqlConn.backendType() == sql::BackendType::sqlite) {
         sql += " RETURNING id, spec_id, inst_spec_id ";
      }

      using dtype = sql::DataType;
      SqlQuery query(this->_sqlConn, sql);

      query.addParam("patId",          dtype::bigint,  dbPatientId);          // patient_id
      query.addParam("seqNno",         dtype::integer, sequenceNo);           // seq_no
      query.addParam("specid0",        dtype::varchar, specId0);              // field 3.0
      query.addParam("specid1",        dtype::varchar, specId1);              // field 3.1
      query.addParam("specid2",        dtype::varchar, specId2);              // field 3.2
      query.addParam("specid3",        dtype::varchar, specId3);              // field 3.3
      query.addParam("instSpecId",     dtype::varchar, insSpecId0);           // field 4.0
      query.addParam("instSpecId1",    dtype::varchar, insSpecId1);           // field 4.1
      query.addParam("testCode",       dtype::varchar, testCode);             // field 5.4
      query.addParam("specCollDt",     dtype::timestamp, specCollDatetime );  // field 8
      query.addParam("specType",       dtype::varchar, specType);             // field 16.0
      query.addParam("specSource",     dtype::varchar, specSource);           // field 16.1
      query.addParam("physicianName",  dtype::varchar, physicianName);        // field 17
      query.addParam("instrument",     dtype::varchar, instrument );          // instrument
      query.addParam("rawdata",        dtype::varchar, rawdata);              // raw_data

      query.addParam("testGroup",      dtype::varchar, testGroup);            // field 5.5
      query.addParam("priorityId",     dtype::varchar, priorityId);           // field 6
      query.addParam("orderedDt",      dtype::timestamp, orderedDateTime);    // field 7
      query.addParam("actionCode",     dtype::varchar, actionCode);           // field 12
      query.addParam("qcid",           dtype::varchar, qcId);                 // field 19.1
      query.addParam("qcExpDt",        dtype::timestamp, qcExpDateTime);      // field 19.2
      query.addParam("qcLotNumber",    dtype::varchar, qcLotNumber);          // field 19.3
      query.addParam("odrFacility",    dtype::varchar, orderingFacility);     // field 21.1
      query.addParam("specDiluFac",    dtype::varchar, specDilutionFactor );  // field 21.2
      query.addParam("rerunAllowed",   dtype::varchar, autoRerunAllowed);     // field 21.3
      query.addParam("reflexAllowed",  dtype::varchar, autoReflexAllowed);    // field 21.4
      query.addParam("parentSpecId",   dtype::varchar, parentSpecId);         // field 22
      query.addParam("resultReported", dtype::timestamp, resultReported );    // field 23
      query.addParam("reportType",     dtype::varchar, reportType);           // field 26
      
      this->throwIfNotSupportedBackend(this->_sqlConn.backendType());
      int64_t dbOrderId = -1;
      if (this->_sqlConn.backendType() == sql::BackendType::mysql)
      {
         int affected = query.execute();
         if (affected > 0)
         {
            dbOrderId     = this->_sqlConn.lastInsertRowid();
            outSpecId     = specId0;
            outInstSpecId = insSpecId0;
         }
      }
      else
      {
         auto sqlResult = query.executeResult();
         if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
         {
            sqlResult->locate(0);
            dbOrderId = sqlResult->getLongLongValue(0);
            if (dbOrderId > 0)
            {
               outSpecId       = sqlResult->getStringValue(1);
               outInstSpecId   = sqlResult->getStringValue(2);
               return dbOrderId;
            }
         }
         else 
            throw AppException("Could not insert data into lis_orders" ); 
      }

      if (dbOrderId <= 0)
         throw AppException("Invalid ID for lis_orders" );
      
      return dbOrderId;
   }

   virtual int64_t saveLisDataRequest(const lis2a::RecordPtr& record, 
      const std::string& instrumentType, 
      int64_t dbMessageId) override
   {
      auto rec = std::static_pointer_cast<lis2a::RequestInfoRecord>(record);
      if (!rec)
         throw AppException("Invalid RequestInfoRecord");

      int         sequenceNo  = rec->sequenceNumber();

      // DevTest use field 3 as order code rannge, separated with repeat delimiter
      // the values will reside in startRange's patientID
      std::string orderCodes  = rec->startRange().patientID();
      std::string startRange1 = orderCodes;
      
      // DevTest uses field 5 as request info status code, the value will reside in testId
      std::string statusCode  = rec->testID().testID();
      std::string rawdata     = rec->toString();

      std::string sql = 
         R"-( INSERT INTO lis_requests ( 
                 header_id, seq_no, start_range1, status_code, raw_data ) 
              VALUES ( :hdrId, :seqno, :start, :status, :raw ) )-";

      using dtype = sql::DataType;
      SqlQuery query(this->_sqlConn, sql);

      query.addParam("hdrId",  dtype::bigint,  dbMessageId);
      query.addParam("seqno",  dtype::integer, sequenceNo);
      query.addParam("start",  dtype::varchar, startRange1);
      query.addParam("status", dtype::varchar, statusCode);
      query.addParam("raw",    dtype::varchar, rawdata);

      int affected = query.executeVoid();
      if (affected > 0)
         return affected;
      else
         throw AppException("Could not insert data into lis_requests" );
   }   

private:

   std::string getPriority(lis2a::OrderPriority value)
   {
      using namespace lis2a;
      if (value == OrderPriority::Stat)
         return "S";
      if (value == OrderPriority::Routine)
         return "R";

      return "";
   }

   std::string getActionCode(lis2a::OrderActionCode value)
   {
      using namespace lis2a;
      if (value == OrderActionCode::Cancel)
         return "C";
      if (value == OrderActionCode::Add)
         return "A";
      if (value == OrderActionCode::QC)
         return "Q";
      if (value == OrderActionCode::Pooled)
         return "D";
      if (value == OrderActionCode::InProcess)
         return "X";
      if (value == OrderActionCode::Pending)
         return "p";

      return "";
   }

   std::string getOrderReportType(lis2a::OrderReportType value)
   {
      using namespace lis2a;

      if (value == OrderReportType::Order)
         return "O";
      if (value == OrderReportType::Correction)
         return "C";
      if (value == OrderReportType::Final)
         return "F";
      if (value == OrderReportType::Cancelled)
         return "X";
      if (value == OrderReportType::Response)
         return "Q";
      if (value == OrderReportType::NoOrder)
         return "Y";
      if (value == OrderReportType::Pending)
         return "I";

      return "";
   }
};

} // namespace svc
} // namespace lis
} // namespace tbs