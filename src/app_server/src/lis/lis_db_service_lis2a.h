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
#include <tobasalis/lis/message.h>
#include <tobasalis/lis2a/message.h>
#include <tobasalis/lis2a/patient.h>
#include <tobasalis/lis2a/subpatient.h>
#include <tobasalis/lis2a/order.h>
#include <tobasalis/lis2a/subuniversaltest.h>
#include <tobasalis/lis2a/requestinfo.h>
#include <tobasalis/lis2a/result.h>
#include <tobasalis/lis2a/terminator.h>
#include <tobasalis/lis2a/comment.h>
#include "dto_lis2a.h"
#include "lis_db_service_base.h"

namespace tbs {
namespace lis {
namespace svc {

struct JobOrder
{
   long id;
   std::string code;
   long patientId;
   long total;
   bool ignore = false;
};

class Lis2aServiceBase : public LisServiceBase
{
public:
   Lis2aServiceBase(const Lis2aServiceBase &) = delete;
   Lis2aServiceBase(Lis2aServiceBase &&) = delete;

   Lis2aServiceBase() {}
   virtual ~Lis2aServiceBase() {}

   virtual bool saveLisData(const lis::MessagePtr& message, DbStoreResultPtr dbStoreResult=nullptr) = 0;

   virtual Json getJobOrderDetail(const svc::JobOrder& jobOrder)= 0;
   virtual Json getJobOrderPatient(const svc::JobOrder& jobOrder)= 0;
   virtual std::vector<JobOrder> getAllJobOrderCode()= 0;
   virtual std::vector<JobOrder> getJobOrderCode(const std::string& orderCode)= 0;
   virtual dto::LIS2AEntryList getLIS2AEntryList(const std::string& instrumentType, const sql::SqlQueryOption& option={})= 0;
   virtual dto::LIS2AResultList getLIS2AResultList(const std::string& instrumentType, int64_t headerId=0)= 0;
};
using Lis2aServicePtr = std::shared_ptr<Lis2aServiceBase>;

template <typename SqlDriverType>
class Lis2aService : public Lis2aServiceBase
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
   Lis2aService(const Lis2aService &) = delete;
   Lis2aService(Lis2aService &&) = delete;

   Lis2aService() {}
   Lis2aService(SqlConnection& conn) : _sqlConn{ conn }  {}
   virtual ~Lis2aService() {}

   virtual bool saveLisData(const lis::MessagePtr& message, DbStoreResultPtr dbStoreResult=nullptr)
   {
      SqlApplyLogId applyLogId(_sqlConn, message->internalId());

      if (message->vendorProtocolId() == lis::MSG_LIS2A)
      {
         // TODO_JEFRI: BEGIN TRANSACTION
         auto lis2aMessage = std::static_pointer_cast<lis2a::Message>(message);
         if ( doSaveLisData(lis2aMessage, dbStoreResult) )
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
         throw AppException("Invalid protocol for LIS2A DB service");

      return false;
   }   

   virtual Json getJobOrderDetail(const svc::JobOrder& jobOrder)
   {
      std::string sql = "SELECT * FROM lis_job_order_details WHERE order_code=:code ORDER BY seq_no ASC";
      SqlQuery query(_sqlConn, sql);
      query.addParam("code", sql::DataType::varchar, jobOrder.code);
      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         Json allOrder;
         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);
            Json order;

            order["order_code"]          = sqlResult->getStringValue("order_code");
            order["spec_id"]             = sqlResult->getStringValue("spec_id");
            order["test_code"]           = sqlResult->getStringValue("test_code");

            std::string specCollDatetime = sqlResult->getStringValue("spec_coll_datetime");
            order["spec_coll_datetime"]  = specCollDatetime.empty() ? "" : sqlResult->getDateTimeValue("spec_coll_datetime").format("{:%Y%m%d%H%M%S}");

            order["spec_type"]           = sqlResult->getStringValue("spec_type");
            order["physician_name"]      = sqlResult->getStringValue("physician_name");
            order["test_group"]          = sqlResult->getStringValue("test_group");
            order["priority_id"]         = sqlResult->getStringValue("priority_id");
            order["ordered_datetime"]    = sqlResult->getStringValue("ordered_datetime");
            order["action_code"]         = sqlResult->getStringValue("action_code");
            //order["qc_id"]             = sqlResult->getStringValue("qc_id");
            //order["qc_exp_datetime"]   = sqlResult->getDateTimeValue("qc_exp_datetime").format("{:%Y%m%d}");
            //order["qc_lot_number"]     = sqlResult->getStringValue("qc_lot_number");
            order["ordering_facility"]   = sqlResult->getStringValue("ordering_facility");
            order["spec_dilution_factor"] = sqlResult->getStringValue("spec_dilution_factor");
            order["auto_rerun_allowed"]  = sqlResult->getStringValue("auto_rerun_allowed");
            order["auto_reflex_allowed"] = sqlResult->getStringValue("auto_reflex_allowed");
            order["parent_spec_id"]      = sqlResult->getStringValue("parent_spec_id");
            //std::string resultReported = sqlResult->getStringValue("result_reported");
            //order["result_reported"]   = resultReported.empty() ? "" : sqlResult->getDateTimeValue("result_reported").format("{:%Y%m%d%H%M%S}");
            order["report_type"]         = sqlResult->getStringValue("report_type");

            allOrder.push_back(order);
         }

         return std::move(allOrder);
      }

      return {};
   }

   virtual Json getJobOrderPatient(const svc::JobOrder& jobOrder)
   {
      std::string sql = R"-(
            SELECT jp.*, jo.order_code FROM lis_job_patients jp
               JOIN lis_job_orders jo ON jo.patient_id = jp.id
            WHERE jo.order_code=:code AND jo.patient_id=:patId )-";

      SqlQuery query(_sqlConn, sql);
      query.addParam("code",  sql::DataType::varchar, jobOrder.code);
      query.addParam("patId", sql::DataType::integer, jobOrder.patientId);

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() == 1)
      {
         Json patient;
         //sqlResult->locate(i);
         patient["practice_patid"] = sqlResult->getStringValue("practice_patid");
         patient["lab_patid"]      = sqlResult->getStringValue("lab_patid");
         patient["gender"]         = sqlResult->getStringValue("gender");
         patient["birthdate"]      = sqlResult->getStringValue("birthdate");
         patient["age"]            = sqlResult->getStringValue("age");
         patient["firstname"]      = sqlResult->getStringValue("firstname");
         patient["middlename"]     = sqlResult->getStringValue("middlename");
         patient["lastname"]       = sqlResult->getStringValue("lastname");
         patient["location_id"]    = sqlResult->getStringValue("location_id");

         return patient;
      }

      return {};
   }

   virtual std::vector<JobOrder> getAllJobOrderCode()
   {
      // there might be exists multiple order(with same order code) for one patient, thus retrieve all order for the patient
      // we only need the last order
      std::string sql = "SELECT * FROM lis_job_orders WHERE closed=0 ORDER BY id ASC";
      SqlQuery query(_sqlConn, sql);
      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         std::vector<JobOrder> allOrder;
         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);

            JobOrder order;
            order.id        = sqlResult->getLongValue("id");
            order.code      = sqlResult->getStringValue("order_code");
            order.patientId = sqlResult->getLongValue("patient_id");
            order.total     = sqlResult->getLongValue("total");

            if (order.code != tbs::NULLSTR || order.code.length() > 0 )
            {
               // mark get only the last inserted order_code
               for (auto it = allOrder.begin(); it < allOrder.end(); ++it)
               {
                  if ( (*it).code == order.code)
                     (*it).ignore = true;
               }

               allOrder.push_back(order);
            }
         }

         return std::move(allOrder);
      }

      return {};
   }

   virtual std::vector<JobOrder> getJobOrderCode(const std::string& orderCode)
   {
      // there might be exists multiple order(with same order code) for one patient, thus retrieve all order for the patient
      // we only need the last order
      std::string sql = "SELECT * FROM lis_job_orders WHERE closed=0 AND order_code=:code ORDER BY id ASC";
      SqlQuery query(_sqlConn, sql);
      query.addParam("code", sql::DataType::varchar, orderCode);
      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         std::vector<JobOrder> allOrder;
         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);

            JobOrder order;
            order.id        = sqlResult->getLongValue("id");
            order.code      = sqlResult->getStringValue("order_code");
            order.patientId = sqlResult->getLongValue("patient_id");
            order.total     = sqlResult->getLongValue("total");

            if (order.code != tbs::NULLSTR || order.code.length() > 0 )
            {
               // mark get only the last inserted order_code
               for (auto it = allOrder.begin(); it < allOrder.end(); ++it)
               {
                  if ( (*it).code == order.code)
                     (*it).ignore = true;
               }

               allOrder.push_back(order);
            }
         }

         return std::move(allOrder);
      }

      return {};
   }

   virtual dto::LIS2AEntryList getLIS2AEntryList(const std::string& instrumentType, const sql::SqlQueryOption& option={})
   {
      bool hasLimitOffset = option.limitOffsetValid();
      bool hasDateRange   = option.dateRangeValid();   
      bool hasFilter      = false;
      std::string limitOffset, dataRange, filter;

      if (instrumentType == lis::DEV_TEST_LIS1A) 
      {
         filter =  " WHERE h.instrument = 'DevTest_LIS1A' OR h.instrument = 'DevTest' ";
         hasFilter = true;
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

      if ( hasLimitOffset )
      {
         limitOffset =  " ORDER BY msg_datetime DESC ";
         limitOffset += Helper::limitAndOffset(option.limit, option.offset, _sqlConn.dbmsName());   
      }

      std::string sql =
         tbsfmt::format(R"-(
            select h_id, msg_datetime, spec_id, spec_type, h_instrument from 
            (
               select h.id as h_id, p.id as p_id, o.id as o_id, r.id as r_id,
                     p.seq_no as p_seq_no, o.seq_no as o_seq_no, r.seq_no as r_seq_no, 
                     h.received, h.msg_datetime, r.completed as r_completed, h.sender as h_sender, h.instrument as h_instrument,
                     p.firstname as p_firstname, p.lastname as p_lastname,
                     o.spec_id, o.test_code, o.spec_type,
                     r.test_code as r_test_code, r.value as r_value, r.flag as r_flag, 
                     r.unit as r_unit, r.range as r_range, r.abnormal_flag as r_abnormal_flag, r.status as r_status, r.operator as r_operator
               from lis_headers h
               JOIN lis_patients p ON h.id = p.header_id
               JOIN lis_orders o ON p.id = o.patient_id
               JOIN lis_results r ON o.id = r.odr_id
               {} {} {}
            ) v_lis2a_tests 
            group by h_id, msg_datetime, spec_id, spec_type, h_instrument
            )-" , filter, dataRange , limitOffset);

      SqlQuery query(_sqlConn, sql);
      if (hasDateRange)
      {
         query.addParam("startDate", sql::DataType::timestamp, option.startDate);
         query.addParam("endDate",   sql::DataType::timestamp, option.endDate);
      } 

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();

      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         dto::LIS2AEntryList list;
         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);
            list.emplace_back( getLIS2AEntry(instrumentType, sqlResult) );
         }

         return list;
      }

      return {};
   }   

   virtual dto::LIS2AResultList  getLIS2AResultList(const std::string& instrumentType, int64_t headerId=0)
   {
      std::string sql = R"-(
         SELECT h.id as h_id, p.id as p_id, o.id as o_id, r.id as r_id,
               p.seq_no as p_seq_no, o.seq_no as o_seq_no, r.seq_no as r_seq_no, 
               h.received, h.msg_datetime, r.completed as r_completed, h.sender as h_sender, h.instrument as h_instrument,
               p.firstname as p_firstname, p.lastname as p_lastname,
               o.spec_id, o.test_code, o.spec_type,
               r.test_code as r_test_code, r.value as r_value, r.flag as r_flag, 
               r.unit as r_unit, r.range as r_range, r.abnormal_flag as r_abnormal_flag, r.status as r_status, r.operator as r_operator
         FROM lis_headers h
         JOIN lis_patients p ON h.id = p.header_id
         JOIN lis_orders o ON p.id = o.patient_id
         JOIN lis_results r ON o.id = r.odr_id )-";

      if (headerId > 0) {
         sql += "  WHERE h.id = :hid ";
      }

      SqlQuery query(_sqlConn, sql);

      if (headerId > 0) {
         query.addParam("hid", sql::DataType::bigint, headerId);
      }   

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         dto::LIS2AResultList list;
         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);
            list.emplace_back( getLIS2AResult(instrumentType, sqlResult) );
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

   virtual std::string getDateTime (const std::string& lisDateTime) override
   {
      std::string result = LisServiceBase::getDateTime(lisDateTime);
      if ( result.empty() )
         return "1900-01-01 00:00:00";
      else
         return result;
   }

   virtual std::string getDate (const std::string& lisDateTime) override
   {
      std::string result = LisServiceBase::getDateTime(lisDateTime);
      if ( result.empty() )
         return "1900-01-01";
      else
         return result;
   }

   virtual std::string translateSampleDescriptor(const std::string& code)
   {
      return code;
   }

   virtual std::string getReferenceRange(const std::string& data, char componentDelimiter) 
   { 
      return data; 
   }

   virtual dto::LIS2AResult getLIS2AResult(const std::string& instrumentType, std::shared_ptr<SqlResult> sqlResult)
   {
      dto::LIS2AResult res;

      res.headerId         = sqlResult->getLongValue("h_id");
      res.patientId        = sqlResult->getLongValue("p_id");
      res.orderId          = sqlResult->getLongValue("o_id");
      res.resultId         = sqlResult->getLongValue("r_id");
      res.msgDatetime      = sqlResult->getStringValue("msg_datetime");
      res.patientName      = sqlResult->getStringValue("p_firstname");
      res.patientLastname  = sqlResult->getStringValue("p_lastname");
      res.specId           = sqlResult->getStringValue("spec_id");
      res.specType         = sqlResult->getStringValue("spec_type");
      res.instrument       = sqlResult->getStringValue("h_instrument");
      res.testCode         = sqlResult->getStringValue("r_test_code");
      res.value            = sqlResult->getStringValue("r_value");
      res.flag             = sqlResult->getStringValue("r_flag");
      res.unit             = sqlResult->getStringValue("r_unit");
      res.range            = sqlResult->getStringValue("r_range");
      res.abnormalFlag     = sqlResult->getStringValue("r_abnormal_flag");
      res.status           = sqlResult->getStringValue("r_status");

      // 2022-12-26 19:31:22.787
      // we want only 2022-12-26 19:31:22
      res.msgDatetime = res.msgDatetime.substr(0, 19);

      return std::move(res);
   }

   virtual dto::LIS2AEntry getLIS2AEntry(const std::string& instrumentType, std::shared_ptr<SqlResult> sqlResult)
   {
      dto::LIS2AEntry ent;

      ent.headerId    = sqlResult->getLongValue("h_id");
      ent.msgDatetime = sqlResult->getStringValue("msg_datetime");
      ent.patientName = "";
      ent.specId      = sqlResult->getStringValue("spec_id");
      ent.specType    = sqlResult->getStringValue("spec_type");
      ent.instrument  = sqlResult->getStringValue("h_instrument");

      return std::move(ent);
   }   

   virtual bool doSaveLisData(const lis2a::MessagePtr& message, DbStoreResultPtr dbStoreResult=nullptr)
   {
      try
      {
         std::string instrumentType = message->instrumentType();
         auto pHeader      = std::static_pointer_cast<lis2a::Record>(message->getHeader());
         auto dbMessageId  = saveLisDataHeader(pHeader, instrumentType);
         auto pHeaderChild = pHeader->getChildren();
      
         while (pHeaderChild)
         {
            // convert pHeaderChild to lis2a::Record
            auto pHeaderChild_ = std::static_pointer_cast<lis2a::Record>(pHeaderChild);

            if (pHeaderChild_->recordType() == lis2a::RecordType::Comment) 
            {
               saveLisDataComment(pHeaderChild_, instrumentType, lis2a::RecordType::Header, dbMessageId);
            }
            else if (pHeaderChild_->recordType() == lis2a::RecordType::Patient)
            {
               auto dbPatientId = saveLisDataPatient(pHeaderChild_, instrumentType, dbMessageId);
               auto patChild = pHeaderChild->getChildren();
               while (patChild)
               {
                  // convert patChild_ to lis2a::Record
                  auto patChild_ = std::static_pointer_cast<lis2a::Record>(patChild);

                  if (patChild_->recordType() == lis2a::RecordType::Comment) {
                     saveLisDataComment(patChild_, instrumentType, lis2a::RecordType::Patient, dbPatientId);
                  }
                  else if (patChild_->recordType() == lis2a::RecordType::Order)
                  {
                     std::string odrSpecId;
                     std::string odrInstSpecId;

                     auto dbOrderId = saveLisDataOrder(patChild_, instrumentType, dbPatientId, odrSpecId, odrInstSpecId);
                     auto orderChild = patChild->getChildren();
                     while (orderChild)
                     {
                        // convert orderChild to lis2a::Record
                        auto orderChild_ = std::static_pointer_cast<lis2a::Record>(orderChild);
                        
                        if (orderChild_->recordType() == lis2a::RecordType::Comment) 
                        {
                           saveLisDataComment(orderChild_, instrumentType, lis2a::RecordType::Order, dbOrderId);
                        }
                        else if (orderChild_->recordType() == lis2a::RecordType::Result) 
                        {
                           saveLisDataResult(orderChild_, instrumentType, dbOrderId,  odrSpecId, odrInstSpecId);
                        }

                        orderChild = orderChild->getNext(); // Orders' children (Result & Comments)
                     }
                  }

                  patChild = patChild->getNext();  // Patients' children (Orders & Comments)
               }
            }
            else if (pHeaderChild_->recordType() == lis2a::RecordType::Query) 
            {
               saveLisDataRequest(pHeaderChild_, instrumentType, dbMessageId);
            }

            pHeaderChild = pHeaderChild->getNext();   // Headers' children (Patients, Comments and Request)
         }

         dbStoreResult->headerId = dbMessageId;
         dbStoreResult->internalMessageId = message->internalId();

         return true;
      }
      catch (const std::exception& ex)
      {
         Logger::logE("[lis_db] [msg:{}] Error occured saving LIS2A data: {}", message->internalId(), ex.what());
      }

      return false;
   }   

   virtual int64_t saveLisDataHeader(const lis2a::RecordPtr& record, const std::string& instrumentType)
   {
      auto header = std::static_pointer_cast<lis2a::HeaderRecord>(record);
      if (!header)
         throw AppException("Invalid HeaderRecord");

      auto        senderIdArr = util::split( header->senderID(), header->componentDelimiter() );
      std::string sender      = senderIdArr.size() > 0 ? util::trim(senderIdArr[0]) : "";
      std::string sender1     = senderIdArr.size() > 1 ? util::trim(senderIdArr[1]) : "";
      std::string sender2     = senderIdArr.size() > 2 ? util::trim(senderIdArr[2]) : "";

      std::string procId      = header->processingID();
      std::string version     = header->version();
      std::string msgDateTime = getDateTime(util::trim(header->messageDateTime()));
      std::string msgCtrlId   = header->messageControlID();
      std::string receiverId  = header->receiverID();
      std::string rawdata     = header->toString();

      std::string outputClauseMSSQL;
      if (_sqlConn.backendType() == sql::BackendType::adodb || _sqlConn.backendType() == sql::BackendType::odbc) {
         outputClauseMSSQL = " OUTPUT inserted.ID ";
      }

      std::string sql = tbsfmt::format(R"-(
            INSERT INTO lis_headers 
               (   received, sender, sender_1, sender_2, processing_id
                  , version, msg_datetime, instrument, message_control_id
                  , receiver_id, raw_data ) {}  
            VALUES ( :rcvd, :sender, :sender1, :sender2, :procId, :version, :msgDt, :inst, :msgcid, :rcvrId, :raw )
         )-", outputClauseMSSQL  );

      if (_sqlConn.backendType() == sql::BackendType::pgsql || _sqlConn.backendType() == sql::BackendType::sqlite) {
         sql += " RETURNING id ";
      }

      using dtype = sql::DataType;
      SqlQuery query(_sqlConn, sql);

      query.addParam("rcvd",    dtype::timestamp, DateTime::now().isoDateTimeString());
      query.addParam("sender",  dtype::varchar, sender);
      query.addParam("sender1", dtype::varchar, sender1);
      query.addParam("sender2", dtype::varchar, sender2);
      query.addParam("procId",  dtype::varchar, procId);
      query.addParam("version", dtype::varchar, version);
      query.addParam("msgDt",   dtype::timestamp, msgDateTime);
      query.addParam("inst",    dtype::varchar, instrumentType);
      query.addParam("msgcid",  dtype::varchar, msgCtrlId );
      query.addParam("rcvrId",  dtype::varchar, receiverId);
      query.addParam("raw",     dtype::varchar, rawdata);

      throwIfNotSupportedBackend(_sqlConn.backendType());
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
            throw AppException("Could not insert message header data into lis_headers" );
      }

      if (dbMessageId <= 0)
         throw AppException("Invalid ID for lis_headers" );

      return dbMessageId;
   }

   virtual int64_t saveLisDataPatient(const lis2a::RecordPtr& record, 
      const std::string& instrumentType, 
      int64_t dbMessageId)
   {
      auto rec = std::static_pointer_cast<lis2a::PatientRecord>(record);
      if (!rec)
         throw AppException("Invalid PatientRecord");

      int         sequenceNo   = rec->sequenceNumber();
      std::string pracPatId    = rec->practiceAssignedPatientID();

      auto        labPatIdArr  = util::split(rec->laboratoryAssignedPatientID(), rec->componentDelimiter());
      std::string labPatId     = labPatIdArr.size() > 0 ? util::trim(labPatIdArr[0]) : "";

      auto        patSexArr    = util::split(rec->getPatientSex(), rec->componentDelimiter());
      std::string gender       = patSexArr.size() > 0 ? util::trim(patSexArr[0]) : "";
      std::string genderFlag   = patSexArr.size() > 1 ? util::trim(patSexArr[1]) : "";

      auto        birthInfoArr = util::split(rec->birthdate(), rec->componentDelimiter());
      std::string birthdatetmp = birthInfoArr.size() > 0 ? util::trim(birthInfoArr[0]) : "";
      std::string birthDate    = getDate(birthdatetmp);
      std::string patAge       = birthInfoArr.size() > 1 ? util::trim(birthInfoArr[1]) : "";
      std::string ageUnit      = birthInfoArr.size() > 2 ? util::trim(birthInfoArr[2]) : "";

      if (ageUnit == "Y") 
         patAge += " Year(s)";
      else if (ageUnit == "M") 
         patAge += " Month(s)";
      else if (ageUnit == "W") 
         patAge += " Week(s)";
      else if (ageUnit == "D") 
         patAge += " Day(s)";
      else if (ageUnit == "H") {
         patAge += " Hour(s)";
      }

      lis2a::PatientName info = rec->getPatientName();
      std::string firstName   = info.firstName();
      std::string middleName  = info.middleName();
      std::string lastName    = info.lastName();
      std::string locationId  = rec->location();
      std::string rawdata     = rec->toString();

      std::string outputClauseMSSQL;
      if (_sqlConn.backendType() == sql::BackendType::adodb || _sqlConn.backendType() == sql::BackendType::odbc) {
         outputClauseMSSQL = " OUTPUT inserted.ID ";
      }

      std::string sql = tbsfmt::format(R"-(
         INSERT INTO lis_patients 
            ( header_id, seq_no, practice_patid, lab_patid, gender, birthdate
            , age, firstname, middlename, lastname, location_id, raw_data ) {} 
         VALUES ( :hdrId, :seqNo, :ppatid, :lpatid, :gender, 
           :birthdate, :age, :firstname, :middlename, :lastname, :locId, :raw ) )-", outputClauseMSSQL );

      if (_sqlConn.backendType() == sql::BackendType::pgsql || _sqlConn.backendType() == sql::BackendType::sqlite) {
         sql += " RETURNING id ";
      }

      using dtype = sql::DataType;
      SqlQuery query(_sqlConn, sql);

      query.addParam("hdrId",      dtype::bigint,  dbMessageId);
      query.addParam("seqNo",      dtype::integer, sequenceNo);
      query.addParam("ppatid",     dtype::varchar, pracPatId);
      query.addParam("lpatid",     dtype::varchar, labPatId);
      query.addParam("gender",     dtype::varchar, gender);
      query.addParam("birthdate",  dtype::date,    birthDate);
      query.addParam("age",        dtype::varchar, patAge);
      query.addParam("firstname",  dtype::varchar, firstName);
      query.addParam("middlename", dtype::varchar, middleName );
      query.addParam("lastname",   dtype::varchar, lastName);
      query.addParam("locId",      dtype::varchar, locationId);
      query.addParam("raw",        dtype::varchar, rawdata);

      throwIfNotSupportedBackend(_sqlConn.backendType());
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
            throw AppException("Could not insert patient data into lis_patients" );
      }

      if (dbPatientId <= 0)
         throw AppException("Invalid ID for lis_patients" );

      return dbPatientId;
   }

   virtual int64_t saveLisDataOrder(const lis2a::RecordPtr& record, 
      const std::string& instrumentType, 
      int64_t dbPatientId, std::string& outSpecId, 
      std::string& outInstSpecId)
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
      std::string insSpecId0 = insSpecArr.size() > 0 ? insSpecArr[0] : "";   // instrument seq number
      std::string insSpecId1 = insSpecArr.size() > 1 ? insSpecArr[1] : "";   // sampling method

      lis2a::UniversalTestID testID = rec->testID();
      std::string testCode = testID.manufacturerCode();

      std::string specCollDatetime = getDateTime(rec->specimenCollectionDateTime());

      std::string specDesc      = rec->specimenDescriptor();                         // field 16
      auto        specDescdArr  = util::split(specDesc, rec->componentDelimiter());
      std::string specTypeTmp   = specDescdArr.size() > 0 ? specDescdArr[0] : "";    // specimen type
      std::string specType      = translateSampleDescriptor(specTypeTmp);
      std::string specSource    = specDescdArr.size() > 1 ? specDescdArr[1] : "";    // specimen source

      std::string physician     = rec->orderingPhysician();
      auto        physicianArr  = util::split(physician, rec->componentDelimiter());
      std::string physicianName = physicianArr.size() > 0 ? physicianArr[0] : "";

      std::string instrument    = instrumentType;
      std::string rawdata       = rec->toString();

      std::string outputClauseMSSQL;
      if (_sqlConn.backendType() == sql::BackendType::adodb || _sqlConn.backendType() == sql::BackendType::odbc) {
         outputClauseMSSQL = " OUTPUT INSERTED.id, INSERTED.spec_id, INSERTED.inst_spec_id ";
      }

      std::string sql = tbsfmt::format(R"-(
         INSERT INTO lis_orders ( 
               patient_id,     seq_no,             spec_id,      spec_id1
            , spec_id2,       spec_id3,           inst_spec_id, inst_spec_id1
            , test_code,      spec_coll_datetime, spec_type,    spec_source
            , physician_name, instrument,         raw_data ) {}  
         VALUES ( :patId, :seqno, :specid0, :specid1, :specid2, :specid3,
            :instSpecId, :instSpecId1, :testCode, :specCollDt, :specType, :specSource, :physician, :inst, :raw )
         )-", outputClauseMSSQL );

      if (_sqlConn.backendType() == sql::BackendType::pgsql || _sqlConn.backendType() == sql::BackendType::sqlite) {
         sql += " RETURNING id, spec_id, inst_spec_id ";
      }

      using dtype = sql::DataType;
      SqlQuery query(_sqlConn, sql);

      query.addParam("patId",       dtype::bigint,  dbPatientId);          // patient_id
      query.addParam("seqno",       dtype::integer, sequenceNo);           // seq_no
      query.addParam("specid0",     dtype::varchar, specId0);              // field 3.0
      query.addParam("specid1",     dtype::varchar, specId1);              // field 3.1
      query.addParam("specid2",     dtype::varchar, specId2);              // field 3.2
      query.addParam("specid3",     dtype::varchar, specId3);              // field 3.3
      query.addParam("instSpecId",  dtype::varchar, insSpecId0);           // field 4.0
      query.addParam("instSpecId1", dtype::varchar, insSpecId1);           // field 4.1
      query.addParam("testCode",    dtype::varchar, testCode);             // field 5.4
      query.addParam("specCollDt",  dtype::timestamp, specCollDatetime );  // field 8
      query.addParam("specType",    dtype::varchar, specType);             // field 16.0
      query.addParam("specSource",  dtype::varchar, specSource);           // field 16.1
      query.addParam("physician",   dtype::varchar, physicianName);        // field 17
      query.addParam("inst",        dtype::varchar, instrument );          // instrument
      query.addParam("raw",         dtype::varchar, rawdata);              // raw_data
      
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

   virtual int64_t saveLisDataResult(const lis2a::RecordPtr& record, 
      const std::string& instrumentType, 
      int64_t dbOrderId, 
      const std::string& odrSpecId, 
      const std::string& odrInstSpecId)
   {
      auto rec = std::static_pointer_cast<lis2a::ResultRecord>(record);
      if (!rec)
         throw AppException("Invalid ResultRecord");

      int         sequenceNo   = rec->sequenceNumber();

      lis2a::UniversalTestID testID = rec->testID();
      std::string testCode     = util::toUpper(testID.manufacturerCode());
      auto        dataArr      = util::split(rec->data(), rec->componentDelimiter());
      std::string value        = dataArr.size() > 0 ? util::trim(dataArr[0]) : "";
      std::string flag         = dataArr.size() > 1 ? util::trim(dataArr[0]) : "";
      std::string unit         = rec->units();
      std::string range        = getReferenceRange(rec->referenceRanges(), rec->componentDelimiter());
      std::string status       = rec->status();
      std::string abnormalFlag = rec->abnormalFlag();
      std::string operatorId   = rec->operatorIdentification();
      std::string completed    = getDateTime(rec->dateTimeCompleted());
      std::string instrumentId = rec->instrumentID();
      std::string rawdata      = rec->toString();

      std::string sql = tbsfmt::format(R"-(
         INSERT INTO lis_results ( 
              odr_id   , odr_spec_id , odr_inst_spec_id , seq_no        , test_code
            , value    , flag        , unit             , range         , abnormal_flag
            , status   , operator    , completed        , instrument_id , raw_data ) 
         VALUES ( :odrId, :odrSpecId, :odrInstSpecId, :seqNo, :testCode, :value, :flag, 
            :unit, :range, :abnormalFlag, :status, :operator, :completed, :instrumentId, :raw ) )-");

      using dtype = sql::DataType;
      SqlQuery query(_sqlConn, sql);

      query.addParam("odrId",         dtype::bigint,  dbOrderId);
      query.addParam("odrSpecId",     dtype::varchar, odrSpecId);
      query.addParam("odrInstSpecId", dtype::varchar, odrInstSpecId);
      query.addParam("seqNo",         dtype::integer, sequenceNo);
      query.addParam("testCode",      dtype::varchar, testCode);
      query.addParam("value",         dtype::varchar, value);
      query.addParam("flag",          dtype::varchar, flag);
      query.addParam("unit",          dtype::varchar, unit);
      query.addParam("range",         dtype::varchar, range );
      query.addParam("abnormalFlag",  dtype::varchar, abnormalFlag);
      query.addParam("status",        dtype::varchar, status);
      query.addParam("operator",      dtype::varchar, operatorId );
      query.addParam("completed",     dtype::timestamp, completed);
      query.addParam("instrumentId",  dtype::varchar, instrumentType);
      query.addParam("raw",           dtype::varchar, rawdata);

      int affected = query.executeVoid();
      if (affected > 0)
         return affected;
      else
         throw AppException("Could not insert data into lis_results" );
   }

   virtual int64_t saveLisDataRequest(const lis2a::RecordPtr& record, 
      const std::string& instrumentType, 
      int64_t dbMessageId)
   {
      auto rec = std::static_pointer_cast<lis2a::RequestInfoRecord>(record);
      if (!rec)
         throw AppException("Invalid RequestInfoRecord");

      int         sequenceNo  = rec->sequenceNumber();
      std::string startRange1 = rec->startRange().patientID();
      std::string startRange2 = rec->startRange().specimenID();
      std::string startRange3 = rec->startRange().reserved();
      std::string testCode    = rec->testID().manufacturerCode();
      std::string statusCode  = rec->requestInfoStatusCode();
      std::string rawdata     = rec->toString();

      std::string sql = R"-(
         INSERT INTO lis_requests ( 
               header_id, seq_no, start_range1, start_range2, start_range3, test_code, status_code, raw_data ) 
         VALUES ( :hdrId, :seqno, :range1, :range2, :range3, :testCode, :statusCode, :raw )
      )-";

      using dtype = sql::DataType;
      SqlQuery query(_sqlConn, sql);

      query.addParam("hdrId",      dtype::bigint,  dbMessageId);
      query.addParam("seqno",      dtype::integer, sequenceNo);
      query.addParam("range1",     dtype::varchar, startRange1);
      query.addParam("range2",     dtype::varchar, startRange2);
      query.addParam("range3",     dtype::varchar, startRange3);
      query.addParam("testCode",   dtype::varchar, testCode);
      query.addParam("statusCode", dtype::varchar, statusCode);
      query.addParam("raw",        dtype::varchar, rawdata);

      int affected = query.executeVoid();
      if (affected > 0)
         return affected;
      else
         throw AppException("Could not insert data into lis_requests" );
   }   

   virtual int64_t saveLisDataComment(const lis2a::RecordPtr& record, 
      const std::string& instrumentType, 
      lis2a::RecordType parentType, 
      int64_t parentDbId)
   {
      auto rec = std::static_pointer_cast<lis2a::CommentRecord>(record);
      if (!rec)
         throw AppException("Invalid CommentRecord");

      int         sequenceNo = rec->sequenceNumber();
      std::string source     = rec->source();
      std::string data       = rec->text();
      auto        cmtTypeArr = util::split(rec->type(), rec->componentDelimiter());
      std::string type0      = cmtTypeArr.size() > 0 ? cmtTypeArr[0] : "";
      std::string type1      = cmtTypeArr.size() > 1 ? cmtTypeArr[1] : "";
      std::string rawdata    = rec->toString();

      std::string ownerType = "N/A";
      if (parentType == lis2a::RecordType::Header)
         ownerType = "Header";
      else if (parentType == lis2a::RecordType::Patient)
         ownerType = "Patient";
      else if (parentType == lis2a::RecordType::Order)
         ownerType = "Order";

      std::string sql = 
         R"-( INSERT INTO lis_comments ( 
                  owner_id, owner_type, seq_no, data, type_0, type_1, raw_data ) 
              VALUES ( :ownerId, :ownerType, :seqno, :data, :type0, :type1, :raw ) )-";

      using dtype = sql::DataType;
      SqlQuery query(_sqlConn, sql);

      query.addParam("ownerId",   dtype::bigint,  parentDbId);
      query.addParam("ownerType", dtype::varchar, ownerType);
      query.addParam("seqno",     dtype::integer, sequenceNo);
      query.addParam("data",      dtype::varchar, data);
      query.addParam("type0",     dtype::varchar, type0);
      query.addParam("type1",     dtype::varchar, type1);
      query.addParam("raw",       dtype::varchar, rawdata);

      int affected = query.executeVoid();
      if (affected > 0)
         return affected;
      else
         throw AppException("Could not insert data into lis_comments" );
   }
};

} // namespace svc
} // namespace lis
} // namespace tbs


