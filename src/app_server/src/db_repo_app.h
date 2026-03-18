#pragma once

#include <vector>
#include <tuple>
#include <tobasa/logger.h>
#include <tobasasql/sql_connection.h>
#include <tobasasql/sql_query.h>
#include <tobasasql/sql_service_base.h>
#include <tobasa/task_metadata.h>

namespace tbs {
namespace app {

class AppDbRepoBase : public sql::SqlServiceBase
{
public:
   AppDbRepoBase() = default;
   virtual ~AppDbRepoBase() = default;
   virtual bool saveTaskData(const TaskMetadataDto& data) = 0;
};

using AppDbRepoPtr = std::shared_ptr<AppDbRepoBase>;

template < typename SqlDriverType >
class AppDbRepo
   : public AppDbRepoBase
{
public:
   using SqlResult     = sql::SqlResult<SqlDriverType>;
   using SqlConnection = sql::SqlConnection<SqlDriverType>;
   using SqlQuery      = sql::SqlQuery<SqlDriverType>;
   using Helper        = typename SqlDriverType::HelperImpl;

private:
   SqlConnection& _sqlConn;

public:
   AppDbRepo() = default;
   virtual ~AppDbRepo() = default;

   AppDbRepo(SqlConnection& conn) : _sqlConn{ conn } {}

   virtual bool saveTaskData(const TaskMetadataDto& data);
};

template < typename SqlDriverType >
bool AppDbRepo<SqlDriverType>::saveTaskData(const TaskMetadataDto& data)
{
   std::string sql = R"-(
      INSERT INTO base_app_task (task_id, name, info, status, result_status, 
               result_message, result_code, start_time, end_time, duration, user_id, app_module)
      VALUES (:task_id, :name, :info, :status, :rstatus, :rmessage, :rcode, :start_time, :end_time, :duration, :user_id, :app_module) )-";
      
   SqlQuery query(_sqlConn, sql);
   query.addParam("task_id",    sql::DataType::integer, data.taskId);
   query.addParam("name",       sql::DataType::varchar, data.name);
   query.addParam("info",       sql::DataType::varchar, data.info);
   query.addParam("status",     sql::DataType::varchar, data.status);
   query.addParam("rstatus",    sql::DataType::varchar, data.resultStatus);
   query.addParam("rmessage",   sql::DataType::varchar, data.resultMessage);
   query.addParam("rcode",      sql::DataType::integer, data.resultCode);
   query.addParam("start_time", sql::DataType::varchar, data.startTime);
   query.addParam("end_time",   sql::DataType::varchar, data.endTime);
   query.addParam("duration",   sql::DataType::varchar, data.duration);
   query.addParam("user_id",    sql::DataType::varchar, data.userId);
   query.addParam("app_module", sql::DataType::varchar, data.appModule);

   bool success = query.executeVoid();
   return success;
}

} // namespace app
} // namespace tbs
