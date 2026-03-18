#pragma once

#include <tobasalis/lis/common.h>
#include <tobasalis/dirui/message_diruih.h>
#include <tobasalis/dirui/record_diruih500.h>
#include <tobasalis/dirui/record_diruihbcc3600.h>
#include "lis_db_service_tempate.h"

namespace tbs {
namespace lis {
namespace svc {

template <typename SqlDriverType>
class LisDirUIService : public LisServiceTemplate<SqlDriverType>
{
public:
   using SqlResult     = sql::SqlResult<SqlDriverType>;
   using SqlConnection = sql::SqlConnection<SqlDriverType>;
   using SqlQuery      = sql::SqlQuery<SqlDriverType>;
   using Helper        = typename SqlDriverType::HelperImpl;
   using SqlApplyLogId = sql::SqlApplyLogId<SqlDriverType>;

public:
   LisDirUIService(const LisDirUIService &) = delete;
   LisDirUIService(LisDirUIService &&) = delete;

   LisDirUIService() {}
   LisDirUIService(SqlConnection& conn) 
      : LisServiceTemplate<SqlDriverType>{ conn }  {}

   virtual ~LisDirUIService() {}

   virtual bool saveLisData(const lis::MessagePtr& message, DbStoreResultPtr dbStoreResult=nullptr) override
   {
      SqlApplyLogId applyLogId(this->_sqlConn, message->internalId());
      std::string instrumentType = message->instrumentType();

      if (instrumentType == lis::DEV_DIRUI_H_500 || instrumentType == lis::DEV_DIRUI_BCC_3600)
      {
         // TODO_JEFRI: BEGIN TRANSACTION
         
         bool success = false;

         if (instrumentType == lis::DEV_DIRUI_H_500)
            success = doSaveLisDataDirUih500(message, dbStoreResult);
         else if (instrumentType == lis::DEV_DIRUI_BCC_3600)
            success = doSaveLisDataDirUiBcc3600(message, dbStoreResult);

         if (success)   
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
         AppException("Invalid protocol for Lis DirUI DB service");

      return false;
   }

protected:

   virtual bool doSaveLisDataDirUih500(const lis::MessagePtr& message, DbStoreResultPtr dbStoreResult)
   {
      try
      {
         auto pHeader = message->getHeader();
         // Should only has one child, so take last child
         auto record = pHeader->getLastChild();
         auto rec = std::static_pointer_cast<dirui::DirUih500Record>(record);
         if (!rec)
            throw AppException("Invalid DirUih500Record");

         int64_t dbHeaderId = 0;
         {
            std::string received = DateTime::now().isoDateTimeString();

            std::string outputClauseMSSQL;
            if (this->_sqlConn.backendType() == sql::BackendType::adodb || this->_sqlConn.backendType() == sql::BackendType::odbc) {
               outputClauseMSSQL = " OUTPUT inserted.ID ";
            }

            std::string sql = tbsfmt::format(R"-(
                     INSERT INTO lis_dirui_headers (received, instrument) {} 
                     VALUES ( :rcvd, :inst ) )-", outputClauseMSSQL );

            if (this->_sqlConn.backendType() == sql::BackendType::pgsql || this->_sqlConn.backendType() == sql::BackendType::sqlite) {
               sql += " RETURNING id ";
            }

            SqlQuery query(this->_sqlConn, sql);
            query.addParam("rcvd", sql::DataType::timestamp, received);
            query.addParam("inst", sql::DataType::varchar,   message->instrumentType());

            this->throwIfNotSupportedBackend(this->_sqlConn.backendType());
            if (this->_sqlConn.backendType() == sql::BackendType::mysql)
            {
               int affected = query.execute();
               if (affected > 0)
                  dbHeaderId = this->_sqlConn.lastInsertRowid();
            }
            else
            {
               auto newId = query.executeScalar();
               if (util::isNumber(newId)) 
                  dbHeaderId = std::stoll(newId);
               else
                  throw AppException("Could not insert message header data into lis_dirui_headers" );
            }

            if (dbHeaderId <= 0 )
               throw AppException("Invalid ID for lis_dirui_headers" );
         }

         std::vector<dirui::DirUih500Fields*>::const_iterator it;
         for (it = rec->getFields().begin(); it != rec->getFields().end(); ++it)
         {
            dirui::DirUih500Fields* fld = *it;
            if (fld)
            {
               std::string fdate, fno, fid, name, field, value, rawdata;

               fdate   = rec->date().empty()  ? "" : rec->date();
               fno     = rec->no().empty()    ? "" : rec->no();
               fid     = rec->id().empty()    ? "" : rec->id();
               name    = fld->name.empty()    ? "" : fld->name;
               field   = fld->field.empty()   ? "" : fld->field;
               value   = fld->value.empty()   ? "" : fld->value;
               rawdata = fld->rawdata.empty() ? "" : fld->rawdata;

               std::string sql = R"-(
                        INSERT INTO lis_dirui_tests ( header_id, fdate, fno, fid, name, field, value, rawdata  )
                        VALUES ( :hdrId, :fdate, :fno, :fid, :name, :field, :value, :raw ) )-";

               SqlQuery query(this->_sqlConn, sql);
               query.addParam("hdrId", sql::DataType::bigint,  dbHeaderId);
               query.addParam("fdate", sql::DataType::varchar, fdate);
               query.addParam("fno",   sql::DataType::varchar, fno);
               query.addParam("fid",   sql::DataType::varchar, fid);
               query.addParam("name",  sql::DataType::varchar, name);
               query.addParam("field", sql::DataType::varchar, field);
               query.addParam("value", sql::DataType::varchar, value);
               query.addParam("raw",   sql::DataType::varchar, rawdata);

               int affected = query.executeVoid();
               if (affected == 0)
                  return false;
            }
         }

         dbStoreResult->headerId = dbHeaderId;
         dbStoreResult->internalMessageId = message->internalId();
         return true;
      }
      catch (const std::exception& ex)
      {
         Logger::logE("[lis_db] [msg:{}] Error occured saving LIS {} data: {}", message->instrumentType(), message->internalId(), ex.what());
      }

      return false;
   }

   virtual bool doSaveLisDataDirUiBcc3600(const lis::MessagePtr& message, DbStoreResultPtr dbStoreResult)
   {
      try
      {
         auto pHeader = message->getHeader();
         // Should only has one child, so take last child
         auto record = pHeader->getLastChild();
         auto rec = std::static_pointer_cast<dirui::RecordDirUiBcc3600>(record);
         if (!rec)
            throw AppException("Invalid RecordDirUiBcc3600");

         int64_t dbHeaderId = -1;
         {
            std::string received = DateTime::now().isoDateTimeString();

            std::string outputClauseMSSQL;
            if (this->_sqlConn.backendType() == sql::BackendType::adodb || this->_sqlConn.backendType() == sql::BackendType::odbc) {
               outputClauseMSSQL = " OUTPUT inserted.ID ";
            }

            std::string sql = tbsfmt::format(R"-(
                     INSERT INTO lis_dirui_headers (received, instrument) {} 
                     VALUES ( :rcvd, :inst ) )-", outputClauseMSSQL );

            if (this->_sqlConn.backendType() == sql::BackendType::pgsql || this->_sqlConn.backendType() == sql::BackendType::sqlite) {
               sql += " RETURNING id ";
            }

            SqlQuery query(this->_sqlConn, sql);
            query.addParam("rcvd",   sql::DataType::timestamp, received);
            query.addParam("inst", sql::DataType::varchar,   message->instrumentType());

            this->throwIfNotSupportedBackend(this->_sqlConn.backendType());
            if (this->_sqlConn.backendType() == sql::BackendType::mysql)
            {
               int affected = query.execute();
               if (affected > 0)
                  dbHeaderId = this->_sqlConn.lastInsertRowid();
            }
            else
            {
               auto newId = query.executeScalar();
               if (util::isNumber(newId)) 
                  dbHeaderId = std::stoll(newId);
               else
                  throw AppException("Could not insert message header data into lis_dirui_headers" );
            }

            if (dbHeaderId <= 0 )
               throw AppException("Invalid ID for lis_dirui_headers" );
         }

         std::vector<dirui::FieldsDirUiBcc3600*>::const_iterator it;
         for (it = rec->getFields().begin(); it != rec->getFields().end(); ++it)
         {
            dirui::FieldsDirUiBcc3600* fld = *it;
            if (fld)
            {
               std::string fdate, fno, fid, name, field, value, rawdata;

               fdate   = rec->date().empty()  ? "" : rec->date();
               fno     = rec->no().empty()    ? "" : rec->no();
               fid     = rec->id().empty()    ? "" : rec->id();
               name    = fld->name.empty()    ? "" : fld->name;
               field   = fld->field.empty()   ? "" : fld->field;
               value   = fld->value.empty()   ? "" : fld->value;
               rawdata = fld->rawdata.empty() ? "" : fld->rawdata;

               std::string sql = 
                  R"-( INSERT INTO lis_dirui_tests ( header_id, fdate, fno, fid, name, field, value, rawdata  )
                       VALUES ( :hdrId, :fdate, :fno, :fid, :name, :field, :value, :raw ) )-";

               SqlQuery query(this->_sqlConn, sql);
               query.addParam("hdrId", sql::DataType::bigint,  dbHeaderId);
               query.addParam("fdate", sql::DataType::varchar, fdate);
               query.addParam("fno",   sql::DataType::varchar, fno);
               query.addParam("fid",   sql::DataType::varchar, fid);
               query.addParam("name",  sql::DataType::varchar, name);
               query.addParam("field", sql::DataType::varchar, field);
               query.addParam("value", sql::DataType::varchar, value);
               query.addParam("raw",   sql::DataType::varchar, rawdata);

               int affected = query.executeVoid();
               if (affected == 0)
                  return false;
            }
         }

         dbStoreResult->headerId = dbHeaderId;
         dbStoreResult->internalMessageId = message->internalId();
         return true;
      }
      catch (const std::exception& ex)
      {
         Logger::logE("[lis_db] [msg:{}] Error occured saving LIS {} data: {}", message->instrumentType(), message->internalId(), ex.what());
      }

      return false;
   }
};

} // namespace svc
} // namespace lis
} // namespace tbs