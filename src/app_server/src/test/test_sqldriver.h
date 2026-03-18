#pragma once

#include <tobasa/logger.h>
#include <tobasa/datetime.h>
#include <tobasa/config.h>
#include <tobasa/variant.h>
#include <tobasasql/sql_table.h>
#include <tobasasql/sql_query.h>
#include <tobasaweb/settings_webapp.h>
#include "test_sql_json_dto.h"
#include "test_sql_ado_defs.h"
#include "test_sql_pgsql_defs.h"
#include "test_sql_sqlite_defs.h"
#include "test_sql_odbc_mssql_defs.h"
#include "test_sql_mysql_defs.h"

namespace tbs {
namespace test {

struct SampleData
{
   std::string valVarchar;
   std::string valText;
   long long   valBigint;
   std::string valChar;
   std::string valDate;
   std::string valTime;
   std::string valDatetime;
   float       valReal;
   double      valDouble;
   std::string valNumeric;
   bool        valBool;
   std::string valBinary;
};


std::vector<SampleData> getSampleDataVector()
{
   std::vector<SampleData> vector;
   vector = {
      //  val_varchar             val_text                      val_bigint            val_char             val_date     val_time     val_datetime       val_real       val_double            val_numeric      val_bool   val_binary
      {u8"こんにちは 1"_asChar, u8"Hello World! QQQ1"_asChar,  111111111111111111LL,  u8"ППППП01"_asChar, "1999-05-20",  "11:20",  "2001-01-24 22:36",  11.123456789,  14.1234567890123456,  "134446666771.45",  true,   "312E20544F42415341"},
      {u8"שלום 2"_asChar,      u8"世界您好！ QQQQQQ2"_asChar,  2222222222222222222LL, u8"ППППП02"_asChar, "2000-07-21", "12:20",   "2002-02-11 19:55",  22.123456789,  24.1234567890123456,  "234446666772.45",  false,  "322E20544F42415341"},
      {u8"Ողջույն 3"_asChar,   u8"Բարև աշխարհ! QQQ3"_asChar,  9223372036854775806LL, u8"ППППП03"_asChar, "2001-04-18",  "03:40",  "2003-03-14 15:21",  33.123456789,  34.1234567890123456,  "334446666773.45",  true,   "332E20544F42415341"},
      {u8"Привет 4"_asChar,    u8"Γεια σου κόσμε! 4"_asChar,  9223372036854775807LL, u8"ППППП04"_asChar,  "2002-04-18", "04:40",  "2004-04-14 15:21",  44.123456789,  44.1234567890123456,  "433444666774.45",  false,  "342E20544F42415341"}
   };

   return std::move(vector);
}

// -------------------------------------------------------

template < typename SqlDriverType>
class TestSqlDriver
{
public:

   // Get correct type from SqlDriverType
   using SqlParameter              = typename SqlDriverType::SqlParameter;
   using SqlParameterCollection    = typename SqlDriverType::SqlParameterCollection;
   using SqlParameterCollectionPtr = typename SqlDriverType::SqlParameterCollectionPtr;
   using VariantType               = typename SqlDriverType::VariantType;
   using VariantHelper             = typename SqlDriverType::VariantHelper;
   using Helper                    = typename SqlDriverType::HelperImpl;

   sql::SqlConnection<SqlDriverType> _sqlConn;
   Json _jsonResult;
   test::SqlTest  _sqlTest;

   TestSqlDriver(std::string jsonTestSource)
   {
      // Get test definitions from JSON
      auto jsonDto = Json::parse(jsonTestSource);
      _sqlTest = jsonDto;
   }

   Json& jsonResult()
   {
      return _jsonResult;
   }

   std::string readMilliseconds(long long milliseconds)
   {
      // Note: https://stackoverflow.com/a/50727882

      auto seconds = milliseconds / 1000;
      milliseconds %= 1000;

      auto minutes = seconds / 60;
      seconds %= 60;

      auto hours = minutes / 60;
      minutes %= 60;

      std::string result;

      if (hours > 0)
         result += std::to_string(hours) + " h ";

      if (minutes>0)
         result += std::to_string(minutes) + " m ";

      if (seconds>0)
         result += std::to_string(seconds) + " s ";

      if (milliseconds>0)
         result += std::to_string(milliseconds) + " ms";

      return result;
   }

   void start()
   {
      using namespace std::chrono;

      long long t_all, t_startConnection, t_runTestInitSampleData, t_runTestInitRawData, t_runTestDirectAndPrepared;
      long long t_runTestQueryWithParameter, t_runTestQuerySelectInsert, t_runTestSqlResult, t_runTestOpenTable, t_runTestModifyTable;

      auto point0 = steady_clock::now();

      startConnection();
      auto point1 = steady_clock::now();
      t_startConnection = (duration_cast<milliseconds>(point1 - point0)).count();

      runTestInitSampleData();
      auto point2 = steady_clock::now();
      t_runTestInitSampleData = (duration_cast<milliseconds>(point2 - point1)).count();

      runTestInitRawData();
      auto point3 = steady_clock::now();
      t_runTestInitRawData = (duration_cast<milliseconds>(point3 - point2)).count();

      runTestDirectAndPrepared();
      auto point4 = steady_clock::now();
      t_runTestDirectAndPrepared = (duration_cast<milliseconds>(point4 - point3)).count();

      runTestQueryWithParameter();
      auto point5 = steady_clock::now();
      t_runTestQueryWithParameter = (duration_cast<milliseconds>(point5 - point4)).count();

      runTestQuerySelectInsert();
      auto point6 = steady_clock::now();
      t_runTestQuerySelectInsert = (duration_cast<milliseconds>(point6 - point5)).count();

      runTestSqlResult();
      auto point7 = steady_clock::now();
      t_runTestSqlResult = (duration_cast<milliseconds>(point7 - point6)).count();

      runTestOpenTable();
      auto point8 = steady_clock::now();
      t_runTestOpenTable = (duration_cast<milliseconds>(point8 - point7)).count();

      runTestModifyTable();
      auto point9 = steady_clock::now();
      t_runTestModifyTable = (duration_cast<milliseconds>(point9 - point8)).count();

      t_all = (duration_cast<milliseconds>(point9 - point0)).count();

      _jsonResult["z_time_all"]                          = readMilliseconds(t_all);
      _jsonResult["z_time_startConnection"]              = readMilliseconds(t_startConnection);
      _jsonResult["z_time_runTestInitSampleData"]        = readMilliseconds(t_runTestInitSampleData);
      _jsonResult["z_time_runTestInitRawData"]           = readMilliseconds(t_runTestInitRawData);
      _jsonResult["z_time_runTestDirectAndPrepared"]     = readMilliseconds(t_runTestDirectAndPrepared);
      _jsonResult["z_time_runTestQueryWithParameter"]    = readMilliseconds(t_runTestQueryWithParameter);
      _jsonResult["z_time_runTestQuerySelectInsert"]     = readMilliseconds(t_runTestQuerySelectInsert);
      _jsonResult["z_time_runTestSqlResult"]             = readMilliseconds(t_runTestSqlResult);
      _jsonResult["z_time_runTestOpenTable"]             = readMilliseconds(t_runTestOpenTable);
      _jsonResult["z_time_runTestModifyTable"]           = readMilliseconds(t_runTestModifyTable);
   }

   void startConnection()
   {
      // Test connection
      auto appOption = Config::getOption<web::conf::Webapp>("webapp");

      _sqlConn.connect(_sqlTest.connString);
      _sqlConn.setLogSqlQuery(appOption.dbConnection.logSqlQuery);
      _sqlConn.setLogSqlQueryInternal(appOption.dbConnection.logInternalSqlQuery);
      _sqlConn.setLogExecuteStatus(appOption.dbConnection.logSqlQuery);

      std::string versionStringSql = _sqlConn.versionString();
      _jsonResult["versionStringTest"] = versionStringSql;
   }


   void runTestDirectAndPrepared()
   {
      // Test insert and select with direct query and prepared parameters

      if (!_sqlTest.option.runTestDirectAndPrepared) {
         return;
      }

      Logger::logD("[test] ---------------------------------- runTestDirectAndPrepared ----------------------------------");

      auto test = _sqlTest.testDirectAndPrepared;
      /*
        Note: SQL query contains unicode characters
        On Windows with odbc, values inserted correcty, on Linux the values get corrupted
        Testing on linux with odbc we use test.preparedMode

        Note: Direct mode sql query
        insertCmd : INSERT INTO sampledata (val_varchar, val_text, val_bigint, val_date) VALUES ( 'ԹԵՍՏ', 'TEST ՔՊ-ի ԱՇԽԱՏԱԿԱԶՄ', 8888888888888888888, '2021-05-28' )
        selectCmd : SELECT val_text, val_varchar, val_bigint, id FROM sampledata WHERE val_bigint = 9223372036854775807 AND val_varchar = 'Привет 4'
      */
      std::string resultConnSelect, resultQuerySelect;
      int resultConnInsert, resultQueryInsert;

      if (test.useDirectMode)
      {
         // Test wth direct sql query

         // test with SqlConnection's executeScalar()
         resultConnSelect = _sqlConn.executeScalar(test.directMode.selectCmd);
         Logger::logD("[test] result Connection scalar: {}", resultConnSelect);

         // test with SqlConnection's execute()
         resultConnInsert = _sqlConn.execute(test.directMode.insertCmd);
         Logger::logD("[test] result Connection cmd: {}", resultConnInsert);

         // test with SqlQuery's executeScalar()
         sql::SqlQuery<SqlDriverType> query1(_sqlConn, test.directMode.selectCmd);
         resultQuerySelect = query1.executeScalar();
         Logger::logD("[test] result Query scalar: {}", resultQuerySelect);

         // test with SqlQuery's execute()
         sql::SqlQuery<SqlDriverType> query2(_sqlConn, test.directMode.insertCmd);
         resultQueryInsert = query2.execute();
         Logger::logD("[test] result Query cmd: {}", resultQueryInsert);
      }
      else
      {
         // Test wth prepared parameter
         SqlParameterCollection paramInsert;
         paramInsert.push_back(std::make_shared<SqlParameter>("varchar", sql::DataType::varchar, test.preparedMode.paramInsertVarchar));
         paramInsert.push_back(std::make_shared<SqlParameter>("text",    sql::DataType::text,    test.preparedMode.paramInsertText));
         paramInsert.push_back(std::make_shared<SqlParameter>("bigint",  sql::DataType::bigint,  test.preparedMode.paramInsertBigint));
         paramInsert.push_back(std::make_shared<SqlParameter>("date",    sql::DataType::date,    test.preparedMode.paramInsertDate));

         SqlParameterCollection paramSelect;
         paramSelect.push_back(std::make_shared<SqlParameter>("varchar", sql::DataType::varchar, test.preparedMode.paramSelectVarchar));

         // test with SqlConnection's executeScalar()
         resultConnSelect = _sqlConn.executeScalar(test.preparedMode.selectCmd, paramSelect);
         Logger::logD("[test] result Connection SELECT: {}", resultConnSelect);

         // test with SqlConnection's execute()
         resultConnInsert = _sqlConn.execute(test.preparedMode.insertCmd, paramInsert);
         Logger::logD("[test] result Connection INSERT: {}", resultConnInsert);

         // -------------------------------------------------------
         // test with SqlQuery's executeScalar()
         sql::SqlQuery<SqlDriverType> query1(_sqlConn, test.preparedMode.selectCmd);
         query1.addParam("varchar", sql::DataType::varchar, test.preparedMode.paramSelectVarchar);
         resultQuerySelect = query1.executeScalar();
         Logger::logD("[test] result Query SELECT: {}", resultQuerySelect);

         // test with SqlQuery's execute()
         sql::SqlQuery< SqlDriverType> query2(_sqlConn, test.preparedMode.insertCmd);
         query2.addParam("varchar", sql::DataType::varchar, test.preparedMode.paramInsertVarchar);
         query2.addParam("text",    sql::DataType::text,    test.preparedMode.paramInsertText);
         query2.addParam("bigint",  sql::DataType::bigint,  test.preparedMode.paramInsertBigint);
         query2.addParam("date",    sql::DataType::date,    test.preparedMode.paramInsertDate);
         resultQueryInsert = query2.execute();
         Logger::logD("[test] result Query INSERT: {}", resultQueryInsert);
      }

      // put test results to JSON
      std::string status1;
      if (_sqlTest.option.unicode) {
         status1 = (resultConnSelect ==  u8"Γεια σου κόσμε! 4"_asStr && resultConnInsert == 1) ? "PASSED" : "FAILED";
      }
      else {
         status1 = (resultConnSelect == "Hello World! 4" && resultConnInsert == 1) ? "PASSED" : "FAILED";
      }

      _jsonResult["runTestDirectAndPrepared"]["SqlConnection"]["Status"]                       = status1;
      _jsonResult["runTestDirectAndPrepared"]["SqlConnection"]["Result_SqlConnection_SELECT"]  = resultConnSelect;
      _jsonResult["runTestDirectAndPrepared"]["SqlConnection"]["Result_SqlConnection_INSERT"]  = resultConnInsert;

      std::string status2;
      if (_sqlTest.option.unicode) {
         status2 = (resultQuerySelect ==  u8"Γεια σου κόσμε! 4"_asStr && resultQueryInsert == 1) ? "PASSED" : "FAILED";
      }
      else {
         status2 = (resultQuerySelect == "Hello World! 4" && resultQueryInsert == 1) ? "PASSED" : "FAILED";
      }

      _jsonResult["runTestDirectAndPrepared"]["SqlQuery"]["Status"]                   = status2;
      _jsonResult["runTestDirectAndPrepared"]["SqlQuery"]["Result_SqllQuery_SELECT"]  = resultQuerySelect;
      _jsonResult["runTestDirectAndPrepared"]["SqlQuery"]["Result_SqlQuery_INSERT"]   = resultQueryInsert;
   }


   void runTestQueryWithParameter()
   {
      // Test Execute query with SqlParameter

      if (!_sqlTest.option.runTestQueryWithParameter) {
         return;
      }

      Logger::logD("[test] ---------------------------------- runQueryWithParameter ----------------------------------");

      auto test = _sqlTest.testQueryWithParameter;

      if (test.selectQueryA.run)
      {
         // Build SqlParameterCollection
         SqlParameterCollection parameters;
         // use cpp's long for sql's integer
         auto paramId   = std::make_shared<SqlParameter>("id",         sql::DataType::integer, test.selectQueryA.param1/*, sizeof(long)*/);
         // use cpp's long long for sql's bigint
         auto paramCode = std::make_shared<SqlParameter>("val_bigint", sql::DataType::bigint,  static_cast<int64_t>(test.selectQueryA.param2)/*, sizeof(long long)*/);
         parameters.push_back(paramId);
         parameters.push_back(paramCode);

         // test SqlConnection's executeScalar()
         std::string result = _sqlConn.executeScalar(test.selectQueryA.cmd, parameters);
         Logger::logD("[test] selectQueryA result scalar : {}", result);

         // test SqlConnection's execute()
         int resultCmd = _sqlConn.execute(test.selectQueryA.cmd, parameters);
         Logger::logD("[test] selectQueryA result execute : {}", resultCmd);

         // put test results to JSON
         std::string status1;
         if ((_sqlConn.backendType() == sql::BackendType::pgsql) /*
             (_sqlConn.backendType() == sql::BackendType::odbc && _sqlTest.option.dbBackend.backendType == "MYSQL")*/ )
         {
            // Note: PGSQL and MYSQL might return affectedRow > 0  for SELECT query
            status1 = (util::startsWith(result, "2003-03-14 15:21") && resultCmd == 1) ? "PASSED" : "FAILED";
         }
         else {
            status1 = (util::startsWith(result, "2003-03-14 15:21") && resultCmd == 0) ? "PASSED" : "FAILED";
         }

         _jsonResult["runQueryWithParameter"]["selectQueryA"]["Status"]          = status1;
         _jsonResult["runQueryWithParameter"]["selectQueryA"]["Result_scalar"]   = result;
         _jsonResult["runQueryWithParameter"]["selectQueryA"]["Result_execute"]  = resultCmd;
      }


      if (test.selectQueryB.run)
      {
         // Just addParam() with sql::SqlQuery
         sql::SqlQuery<SqlDriverType> query(_sqlConn, test.selectQueryB.cmd);
         // use cpp's long for sql's integer
         query.addParam("int", sql::DataType::integer, test.selectQueryB.param1);
         // use std::string for sql's varchar
         query.addParam("str", sql::DataType::varchar, test.selectQueryB.param2/*, 8*/);
         std::string resScalar  = query.executeScalar();
         Logger::logD("[test] selectQueryB result scalar : {}", resScalar);

         int resExe = query.execute();
         Logger::logD("[test] selectQueryB result execute : {}", resExe);

         // put test results to JSON
         std::string status2;
         if ((_sqlConn.backendType() == sql::BackendType::pgsql) /*||
             (_sqlConn.backendType() == sql::BackendType::odbc && _sqlTest.option.dbBackend.backendType == "MYSQL")*/ )
         {
            // Note: PGSQL and MYSQL might return affectedRow > 0  for SELECT query
            status2 = (util::startsWith(resScalar, "2002-02-11 19:55") && resExe == 1) ? "PASSED" : "FAILED";
         }
         else {
            status2 = (util::startsWith(resScalar, "2002-02-11 19:55") && resExe == 0) ? "PASSED" : "FAILED";
         }

         _jsonResult["runQueryWithParameter"]["selectQueryB"]["Status"]          = status2;
         _jsonResult["runQueryWithParameter"]["selectQueryB"]["Result_scalar"]   = resScalar;
         _jsonResult["runQueryWithParameter"]["selectQueryB"]["Result_execute"]  = resExe;
      }
   }


   void runTestQuerySelectInsert()
   {
      // Test Execute Select and Insert with SqlParameter

      if (!_sqlTest.option.runTestQuerySelectInsert) {
         return;
      }

      Logger::logD("[test] ################## testQuerySelectInsert ##################");

      auto test = _sqlTest.testQuerySelectInsert;

      if (test.selectQueryB.run)
      {
         std::string resultSelectNote, resutlSelectRawdata;
         std::string prmInsertNote    = u8ToString(u8"αβγδ");
         std::string prmInsertRawdata = u8ToString(u8"महसुस");

         sql::SqlQuery<SqlDriverType> query1(_sqlConn, test.selectQueryB.cmd);
         // use cpp's long for sql's integer
         query1.addParam("id",          sql::DataType::integer, test.selectQueryB.param1/*, sizeof(long)*/);
         query1.addParam("val_varchar", sql::DataType::varchar, test.selectQueryB.param2/*, 10*/);
         auto sqlResult = query1.executeResult();

         // sqlResult must valid and has row(s)
         if (sqlResult->isValid() && sqlResult->totalRows() > 0)
         {
            resultSelectNote    = sqlResult->getStringValue("val_varchar");
            resutlSelectRawdata = sqlResult->getStringValue("val_text");

            if (test.useParamFromJson)
            {
               prmInsertNote    = test.insertQuery.param1;
               prmInsertRawdata = test.insertQuery.param2;
            }
            else
            {
               prmInsertNote    = resultSelectNote;
               prmInsertRawdata = resutlSelectRawdata;
            }

            // Use sql::SqlQuery
            sql::SqlQuery<SqlDriverType> query2(_sqlConn, test.insertQuery.cmd);
            query2.addParam("val_varchar", sql::DataType::varchar, prmInsertNote/*,    10*/);
            query2.addParam("val_text",    sql::DataType::text,    prmInsertRawdata/*, 100*/);
            int rowsAffected = query2.execute();

            std::string status1;
            if (_sqlTest.option.unicode) {
               status1 = (resultSelectNote == u8ToString(u8"Привет 4") && resutlSelectRawdata == u8ToString(u8"Γεια σου κόσμε! 4") && rowsAffected == 1) ? "PASSED" : "FAILED";
            }
            else {
               status1 = (resultSelectNote == "HELLO 4" && resutlSelectRawdata == "Hello World! 4" && rowsAffected == 1) ? "PASSED" : "FAILED";
            }

            _jsonResult["runQuerySelectInsert"]["selectQueryB"]["Status"]               = status1;
            _jsonResult["runQuerySelectInsert"]["selectQueryB"]["Result_SelectNote"]    = resultSelectNote;
            _jsonResult["runQuerySelectInsert"]["selectQueryB"]["Result_SelectRawdata"] = resutlSelectRawdata;

            std::string status2 = (rowsAffected == 1) ? "PASSED" : "FAILED";
            _jsonResult["runQuerySelectInsert"]["insertQuery"]["Status"]                = status2;
            _jsonResult["runQuerySelectInsert"]["insertQuery"]["Result_rowsAffected"]   = rowsAffected;
         }
      }

      if (test.insertQueryB.run)
      {
         sql::SqlQuery<SqlDriverType> query(_sqlConn,test.insertQueryB.cmd);

         query.addParam("bigint",  sql::DataType::bigint,     test.insertQueryB.paramBigInt);
         query.addParam("char",    sql::DataType::character,  test.insertQueryB.paramChar);
         query.addParam("bool",    sql::DataType::boolean,    test.insertQueryB.paramBool       /*, sizeof(bool)*/);
         query.addParam("float",   sql::DataType::float4,     test.insertQueryB.paramFloat      /*, sizeof(float)*/);
         query.addParam("double",  sql::DataType::float8,     test.insertQueryB.paramDouble     /*, sizeof(double)*/);
         // use std::string for sql's numeric type, no need to include size,direction and decimal digit
         query.addParam("numeric", sql::DataType::numeric,    test.insertQueryB.paramNumeric/*, 14, sql::ParameterDirection::input, 2*/);
         // use std::string for sql's date,  format is yyyy-mm-dd
         query.addParam("date",    sql::DataType::date,       test.insertQueryB.paramDate);
         // use std::string for sql's time,  format is hh:mm:ss
         query.addParam("time",    sql::DataType::time,       test.insertQueryB.paramTime);
         // use std::string for sql's timestamp,  format is yyyy-mm-dd hh:mm:ss
         query.addParam("dttime",  sql::DataType::timestamp,  test.insertQueryB.paramDateTime);
         // use Hexadecimal encoded binary string for sql's varbinary, and specify raw data's size
         query.addParam("binary",  sql::DataType::varbinary,  test.insertQueryB.paramBinary,  static_cast<long>(test.insertQueryB.paramBinary.length()/2) );   // 4150504C49434154494F4E = APPLICATION
         // use Hexadecimal encoded binary string for sql's varbinary, and specify raw data's size
         query.addParam("binary1", sql::DataType::varbinary,  test.insertQueryB.paramBinary1, static_cast<long>(test.insertQueryB.paramBinary1.length()/2) );

         int rowsAffected = query.execute();
         auto sqlResult   = query.executeResult();

         std::string status = (rowsAffected == 1 && sqlResult->affectedRows() == 1) ? "PASSED" : "FAILED";
         _jsonResult["runQuerySelectInsert"]["insertQueryB"]["Status"] = status;
         _jsonResult["runQuerySelectInsert"]["insertQueryB"]["Result_Execute_rowsAffected"] = rowsAffected;
         _jsonResult["runQuerySelectInsert"]["insertQueryB"]["Result_ExecuteResult_rowsAffected"] = sqlResult->affectedRows();
      }
   }


   void runTestSqlResult()
   {
      // Test SqlResult object

      if (!_sqlTest.option.runTestSqlResult) {
         return;
      }

      Logger::logD("[test] ---------------------------------- testSqlResult ----------------------------------");

      auto test = _sqlTest.testSqlResult;
      std::string selectQuery;
      SqlParameterCollection parameters;

      sql::SqlResult<SqlDriverType> sqlResult(_sqlConn);
      sqlResult.setOptionCacheData(test.optionCache);
      sqlResult.setOptionOpenTable(test.optionTable);

      if (test.optionTable)
      {
         // retrieve as table
         selectQuery = test.tableName;
      }
      else
      {
         selectQuery = test.selectQueryA.cmd;
         if (test.useParameter)
         {
            auto paramCode = std::make_shared<SqlParameter>("code", sql::DataType::integer, test.selectQueryA.param1/*, sizeof(long)*/);
            parameters.push_back(paramCode);
         }
      }

      sqlResult.runQuery(selectQuery, parameters);

      // sql result must valid and has row(s)
      if (sqlResult.isValid() && sqlResult.totalRows() > 0)
      {
         long totalRows       = sqlResult.totalRows();
         sqlResult.moveLast();

         VariantType vId      = sqlResult.getVariantValue("id");
         VariantType vNote    = sqlResult.getVariantValue("note");
         VariantType vRawdata = sqlResult.getVariantValue("rawdata");
         auto vIdStr          = VariantHelper::toString(vId);
         auto vNoteStr        = VariantHelper::toString(vNote);
         auto vRawdataStr     = VariantHelper::toString(vRawdata);
         auto valNote         = sqlResult.getStringValue(1);


         Logger::logD("[test] vIdStr= {}, vNoteStr= {}, vRawdataStr= {}, valNote= {}", vIdStr, vNoteStr, vRawdataStr.substr(0, 30), valNote);

         std::string status;

         if (test.optionTable)
         {
            long nRows = _sqlTest.testInitSampleTables.rawDataTotalRow; // default 10000 rows
            
            long code = nRows;
            if (nRows >0 && nRows <= 10)
               code = 10;
            else if (nRows > 10 && nRows <= 100)
               code = 100;

            std::string nRowsStr = std::to_string(code);
            std::string note10000 = "NOTE_" + nRowsStr;

            status = (totalRows == nRows && vIdStr == nRowsStr && vNoteStr == note10000 && valNote == note10000 && vRawdataStr.size() == 1024) ? "PASSED" : "FAILED";
         }
         else {
            status = (totalRows == 100 && vIdStr == "100" && vNoteStr == "NOTE_100" && valNote == "NOTE_100" && vRawdataStr.size() == 1024) ? "PASSED" : "FAILED";
         }
         _jsonResult["runQuerySelectInsert"]["testSqlResult"]["Status"]                 = status;
         _jsonResult["runQuerySelectInsert"]["testSqlResult"]["Result_totalRows"]       = totalRows;
         _jsonResult["runQuerySelectInsert"]["testSqlResult"]["Result_Variant_Id"]      = vIdStr;
         _jsonResult["runQuerySelectInsert"]["testSqlResult"]["Result_Variant_Note"]    = vNoteStr;
         _jsonResult["runQuerySelectInsert"]["testSqlResult"]["Result_Variant_Rawdata"] = vRawdataStr.substr(0, 30);
         _jsonResult["runQuerySelectInsert"]["testSqlResult"]["Result_Note"]            = valNote;
      }
   }


   void runTestOpenTable()
   {
      // Test SqlTable, open

      if (!_sqlTest.option.runTestOpenTable) {
         return;
      }

      Logger::logD("[test] -- testOpenTable --");

      auto test                = _sqlTest.testOpenTable;
      std::string tableName    = test.tableName;
      std::string customSelect = test.customSelect;

      sql::SqlTable < SqlDriverType > table(_sqlConn, tableName, customSelect);

      table.getRetrieveDataOption()
           .pageSize(test.option.pageSize)    // 10 records per page
           .pagePosition(test.option.pagePos) // set page position
           .orderBy(test.option.orderBy);

      table.init();
      if (table.isValid())
      {
         // Note:
         // for null field, we store as std::string variant with value "null"
         // For nullable column, always check for isnull, before getting its non string value with std::get<T>(variant val)

         // -------------------------------------------------------
         table.moveFirst();

         Logger::logD("[test] note= {}, rawdata= {}, code= {}, val_datetime= {}",
            table.getStringValue("val_varchar"), table.getStringValue("val_text"),
            table.getStringValue("val_bigint"),  table.getStringValue("val_datetime"));

         std::string status0;
         if (_sqlTest.option.unicode)
         {
            status0 = (table.getStringValue("val_varchar") ==  u8ToString(u8"こんにちは 1")
                       && table.getStringValue("val_text") ==  u8ToString(u8"Hello World! QQQ1")
                       && std::get<int64_t>(table.getVariantValue("val_bigint")) == 111111111111111111LL
                       && util::startsWith(table.getStringValue("val_datetime"), "2001-01-24 22:36")) ? "PASSED" : "FAILED";
         }
         else
         {
            status0 = (table.getStringValue("val_varchar") == "HELLO 1"
                       && table.getStringValue("val_text") == "Hello World! 1"
                       && std::get<int64_t>(table.getVariantValue("val_bigint")) == 111111111111111111LL
                       && util::startsWith(table.getStringValue("val_datetime"), "2001-01-24 22:36")) ? "PASSED" : "FAILED";
         }

         _jsonResult["testOpenTable"]["moveFirst"]["Status"]              = status0;
         _jsonResult["testOpenTable"]["moveFirst"]["Result_val_varchar"]  = table.getStringValue("val_varchar");
         _jsonResult["testOpenTable"]["moveFirst"]["Result_val_text"]     = table.getStringValue("val_text");
         _jsonResult["testOpenTable"]["moveFirst"]["Result_val_bigint"]   = std::get<int64_t>(table.getVariantValue("val_bigint"));
         _jsonResult["testOpenTable"]["moveFirst"]["Result_val_datetime"] = table.getStringValue("val_datetime");

         // -------------------------------------------------------
         table.moveNext();

         Logger::logD("[test] note= {}, rawdata= {}, code= {}, val_datetime= {}",
            table.getStringValue("val_varchar"), table.getStringValue("val_text"),
            table.getStringValue("val_bigint"),  table.getStringValue("val_datetime"));

         std::string status1;
         if (_sqlTest.option.unicode)
         {
            status1 = (table.getStringValue("val_varchar") ==  u8ToString(u8"שלום 2")
                       && table.getStringValue("val_text") ==  u8ToString(u8"世界您好！ QQQQQQ2")
                       && std::get<int64_t>(table.getVariantValue("val_bigint")) == 2222222222222222222
                       && util::startsWith(table.getStringValue("val_datetime"), "2002-02-11 19:55")) ? "PASSED" : "FAILED";
         }
         else
         {
            status1 = (table.getStringValue("val_varchar") == "HELLO 2"
                       && table.getStringValue("val_text") == "Hello World! 2"
                       && std::get<int64_t>(table.getVariantValue("val_bigint")) == 2222222222222222222
                       && util::startsWith(table.getStringValue("val_datetime"), "2002-02-11 19:55")) ? "PASSED" : "FAILED";
         }
         _jsonResult["testOpenTable"]["moveNext1"]["Status"]              = status1;
         _jsonResult["testOpenTable"]["moveNext1"]["Result_val_varchar"]  = table.getStringValue("val_varchar");
         _jsonResult["testOpenTable"]["moveNext1"]["Result_val_text"]     = table.getStringValue("val_text");
         _jsonResult["testOpenTable"]["moveNext1"]["Result_val_bigint"]   = std::get<int64_t>(table.getVariantValue("val_bigint"));
         _jsonResult["testOpenTable"]["moveNext1"]["Result_val_datetime"] = table.getStringValue("val_datetime");

         // -------------------------------------------------------
         table.moveNext();

         Logger::logD("[test] note= {}, rawdata= {}, code= {}, val_datetime= {}",
            table.getStringValue("val_varchar"), table.getStringValue("val_text"),
            table.getStringValue("val_bigint"),  table.getStringValue("val_datetime"));

         std::string status2;
         if (_sqlTest.option.unicode)
         {
            status2 = (table.getStringValue("val_varchar") ==  u8ToString(u8"Ողջույն 3")
                       && table.getStringValue("val_text") ==  u8ToString(u8"Բարև աշխարհ! QQQ3")
                       && std::get<int64_t>(table.getVariantValue("val_bigint")) == 9223372036854775806
                       && util::startsWith(table.getStringValue("val_datetime"), "2003-03-14 15:21")) ? "PASSED" : "FAILED";
         }
         else
         {
            status2 = (table.getStringValue("val_varchar") == "HELLO 3"
                       && table.getStringValue("val_text") == "Hello World! 3"
                       && std::get<int64_t>(table.getVariantValue("val_bigint")) == 9223372036854775806
                       && util::startsWith(table.getStringValue("val_datetime"), "2003-03-14 15:21")) ? "PASSED" : "FAILED";
         }
         _jsonResult["testOpenTable"]["moveNext2"]["Status"]              = status2;
         _jsonResult["testOpenTable"]["moveNext2"]["Result_val_varchar"]  = table.getStringValue("val_varchar");
         _jsonResult["testOpenTable"]["moveNext2"]["Result_val_text"]     = table.getStringValue("val_text");
         _jsonResult["testOpenTable"]["moveNext2"]["Result_val_bigint"]   = std::get<int64_t>(table.getVariantValue("val_bigint"));
         _jsonResult["testOpenTable"]["moveNext2"]["Result_val_datetime"] = table.getStringValue("val_datetime");
      }
   }


   void runTestModifyTable()
   {
      // Test SqlTable Insert, Update, Delete data

      if (!_sqlTest.option.runTestModifyTable) {
         return;
      }

      DateTime dt;
      std::string dtNow = dt.isoDateTimeString();

      Logger::logD("[test] -- testModifyTable --");

      auto test = _sqlTest.testModifyTable;
      auto paramSelectId = std::make_shared<SqlParameter>("id", sql::DataType::integer, test.paramSelectId/*, sizeof(int)*/);

      sql::SqlTable < SqlDriverType > table(_sqlConn, test.tableName);
      table.getRetrieveDataOption()
           .pageSize(test.option.pageSize)              // 10 records per page
           .pagePosition(test.option.pagePos)           // set page position
           .orderBy(test.option.orderBy)
           .getParameters().push_back(paramSelectId);   // sql query now has one parameter => id

      table.init();

      if (table.isValid())
      {
      #if 0
         // -------------------------------------------------------
         // TEST INSERT WITH _variant_t wrapped in VariantType
         if (test.insertWithNativeVariant)
         {
            _variant_t vtPrmId;
            vtPrmId.vt = VT_I2;
            vtPrmId.iVal = test.paramId;

            _variant_t vtPrmNote;
            vtPrmNote = util::utf8_to_bstr_t(test.paramNote);       //_bstr_t((char*)test.paramNote.c_str()); // note: cast to (char*)

            _variant_t vtPrmRawdata;
            vtPrmRawdata = util::utf8_to_bstr_t(test.paramRawdata);//_bstr_t(static_cast<const char*>(test.paramRawdata.c_str()) ); // note: cast to (char*)

            _variant_t vtPrmCode;
            vtPrmCode.vt = VT_I2;
            vtPrmCode.iVal = test.paramCode;

            table.appendRows(1);
            table.moveLast();

            table.setValue("id",          VariantType(vtPrmId));
            table.setValue("val_varchar", VariantType(vtPrmNote));
            table.setValue("val_text",    VariantType(vtPrmRawdata));
            table.setValue("val_bigint",  VariantType(vtPrmCode));
            table.saveTable();
         }
         // -------------------------------------------------------
      #endif

         DateTime datetime;
         datetime.parse(test.paramDateTime);

         // TEST INSERT VariantType, make sure appendRows() first!
         //datetime.timePoint() += std::chrono::years{ 1 } + std::chrono::months{ 1 } + std::chrono::days{ 1 } + hours{ 1 } + minutes{ 1 } + seconds{1};
         datetime.timePoint() += tbsdate::years{ 1 };

         table.appendRows(1);
         table.moveLast();
         table.setValue("val_bigint",           VariantType(static_cast<int64_t>(test.paramBigInt + 111) ));
         table.setValue("val_varchar",          VariantType(test.paramVarChar + "111"));
         table.setValue("val_text",             VariantType(test.paramText + "111"));
         table.setValue("val_datetime",         VariantType(datetime.format("{:%Y-%m-%d %H:%M:%S}")));
         table.setValue("val_date",             VariantType(datetime.format("{:%Y-%m-%d}")));
         table.setValue("val_time",             VariantType(datetime.format("{:%H:%M:%S}")));
         table.setValue("val_char",             VariantType(test.paramChar + "111" ));
         table.setValue("val_real",             VariantType(test.paramFloat + 1));
         table.setValue("val_double",           VariantType(test.paramDouble + 1));
         table.setValue("val_numeric",          VariantType(test.paramNumeric));
         table.setValue("val_bool",             VariantType(test.paramBool));
         table.saveTable();


         // TEST INSERT
         datetime.timePoint() += tbsdate::months{ 1 };

         table.appendRows(1);
         table.moveLast();
         table.setValueBigInteger("val_bigint", static_cast<int64_t>(test.paramBigInt + 222) );
         table.setValueString("val_varchar",    test.paramVarChar + "222");
         table.setValueString("val_text",       test.paramText + "222");
         table.setValueString("val_datetime",   datetime.format("{:%Y-%m-%d %H:%M:%S}"));
         table.setValueString("val_date",       datetime.format("{:%Y-%m-%d}"));
         table.setValueString("val_time",       datetime.format("{:%H:%M:%S}"));
         table.setValueString("val_char",       test.paramChar + "222");
         table.setValueFloat("val_real",        test.paramFloat + 2);
         table.setValueDouble("val_double",     test.paramDouble + 2);
         table.setValueString("val_numeric",    test.paramNumeric);
         table.setValueBool("val_bool",         test.paramBool);

         table.saveTable();


         // TEST UPDATE, make sure table has row to update!
         table.reloadData();
         if (table.isValid() && table.totalRows() > 0)
         {
            datetime.timePoint() += tbsdate::days{ 1 };

            table.moveFirst();
            table.setValueBigInteger("val_bigint", test.paramBigInt+333);
            table.setValueString("val_varchar",    test.paramVarChar + "333");
            table.setValueString("val_text",       test.paramText + "333");
            table.setValueString("val_datetime",   datetime.format("{:%Y-%m-%d %H:%M:%S}"));
            table.setValueString("val_date",       datetime.format("{:%Y-%m-%d}"));
            table.setValueString("val_time",       datetime.format("{:%H:%M:%S}"));
            table.setValueString("val_char",       test.paramChar + "333");
            table.setValueFloat("val_real",        test.paramFloat + 3);
            table.setValueDouble("val_double",     test.paramDouble + 3);
            table.setValueString("val_numeric",    test.paramNumeric);
            table.setValueBool("val_bool",         test.paramBool);
            table.saveTable();
         }

         table.getRetrieveDataOption().getParameters().clear(); // clear SELECT parameter, so we get all rows
         table.reloadData();
         if (table.isValid())
         {

            // -------------------------------------------------------
            table.moveFirst();

            Logger::logD("[test] note= {}, rawdata= {}, code= {}, val_datetime= {}",
               table.getStringValue("val_varchar"), table.getStringValue("val_text").substr(0, 20),
               table.getStringValue("val_bigint"), table.getStringValue("val_datetime"));

            std::string status;
            if (_sqlTest.option.unicode)
            {
               status = (table.getStringValue("val_varchar") ==  u8ToString(u8"ꦲꦭꦺꦴ333")
                         && table.getStringValue("val_text") ==  u8ToString(u8"77 ꦏꦢꦺꦴꦱ꧀ꦥꦸꦤ꧀ꦢꦶꦏꦧꦂꦫꦶꦥꦸꦤ? 333")
                         && std::get<int64_t>(table.getVariantValue("val_bigint")) == 9000000000000000333
                         && util::startsWith(table.getStringValue("val_datetime"), "2002-01-31 16:18:18")) ? "PASSED" : "FAILED";
            }
            else
            {
               status = (table.getStringValue("val_varchar") == "Welcome333"
                         && table.getStringValue("val_text") == "77 Welcome Selamat Datang333"
                         && std::get<int64_t>(table.getVariantValue("val_bigint")) == 9000000000000000333
                         && util::startsWith(table.getStringValue("val_datetime"), "2002-01-31 16:18:18")) ? "PASSED" : "FAILED";
            }
            _jsonResult["testModifyTable"]["moveFirst"]["Status"]              = status;
            _jsonResult["testModifyTable"]["moveFirst"]["Result_val_varchar"]  = table.getStringValue("val_varchar");
            _jsonResult["testModifyTable"]["moveFirst"]["Result_val_text"]     = table.getStringValue("val_text");
            _jsonResult["testModifyTable"]["moveFirst"]["Result_val_bigint"]   = std::get<int64_t>(table.getVariantValue("val_bigint"));
            _jsonResult["testModifyTable"]["moveFirst"]["Result_val_datetime"] = table.getStringValue("val_datetime");
            _jsonResult["testModifyTable"]["moveFirst"]["Result_val_numeric"]  = table.getStringValue("val_numeric");


            // -------------------------------------------------------
            table.moveLast();

            Logger::logD("[test] note= {}, rawdata= {}, code= {}, val_datetime= {}",
               table.getStringValue("val_varchar"), table.getStringValue("val_text").substr(0, 20),
               table.getStringValue("val_bigint"), table.getStringValue("val_datetime"));

            std::string status0;
            if (_sqlTest.option.unicode)
            {
               status0 = (table.getStringValue("val_varchar") ==  u8ToString(u8"ꦲꦭꦺꦴ222")
                          && table.getStringValue("val_text") ==  u8ToString(u8"77 ꦏꦢꦺꦴꦱ꧀ꦥꦸꦤ꧀ꦢꦶꦏꦧꦂꦫꦶꦥꦸꦤ? 222")
                          && std::get<int64_t>(table.getVariantValue("val_bigint")) == 9000000000000000222
                          && util::startsWith(table.getStringValue("val_datetime"), "2002-01-30 16:18:18")) ? "PASSED" : "FAILED";
            }
            else
            {
               status0 = (table.getStringValue("val_varchar") == "Welcome222"
                          && table.getStringValue("val_text") == "77 Welcome Selamat Datang222"
                          && std::get<int64_t>(table.getVariantValue("val_bigint")) == 9000000000000000222LL
                          && util::startsWith(table.getStringValue("val_datetime"), "2002-01-30 16:18:18")) ? "PASSED" : "FAILED";
            }
            _jsonResult["testModifyTable"]["moveLast"]["Status"]              = status0;
            _jsonResult["testModifyTable"]["moveLast"]["Result_val_varchar"]  = table.getStringValue("val_varchar");
            _jsonResult["testModifyTable"]["moveLast"]["Result_val_text"]     = table.getStringValue("val_text");
            _jsonResult["testModifyTable"]["moveLast"]["Result_val_bigint"]   = std::get<int64_t>(table.getVariantValue("val_bigint"));
            _jsonResult["testModifyTable"]["moveLast"]["Result_val_datetime"] = table.getStringValue("val_datetime");
            _jsonResult["testModifyTable"]["moveLast"]["Result_val_numeric"]  = table.getStringValue("val_numeric");

            // -------------------------------------------------------
            if (! table.isBof())
            {
               table.movePrevious();

               Logger::logD("[test] note= {}, rawdata= {}, code= {}, val_datetime= {}",
                  table.getStringValue("val_varchar"), table.getStringValue("val_text").substr(0, 20),
                  table.getStringValue("val_bigint"), table.getStringValue("val_datetime"));

               std::string status1;
               if (_sqlTest.option.unicode)
               {
                  status1 = (table.getStringValue("val_varchar") ==  u8ToString(u8"ꦲꦭꦺꦴ111")
                             && table.getStringValue("val_text") ==  u8ToString(u8"77 ꦏꦢꦺꦴꦱ꧀ꦥꦸꦤ꧀ꦢꦶꦏꦧꦂꦫꦶꦥꦸꦤ? 111")
                             && table.getStringValue("val_bigint") == "9000000000000000111"
                             && util::startsWith(table.getStringValue("val_datetime"), "2001-12-31 05:49:12")) ? "PASSED" : "FAILED";
               }
               else
               {
                  status1 = (table.getStringValue("val_varchar") == "Welcome111"
                             && table.getStringValue("val_text") == "77 Welcome Selamat Datang111"
                             && table.getStringValue("val_bigint") == "9000000000000000111"
                             && util::startsWith(table.getStringValue("val_datetime"), "2001-12-31 05:49:12")) ? "PASSED" : "FAILED";
               }
               _jsonResult["testModifyTable"]["movePrevious1"]["Status"]              = status1;
               _jsonResult["testModifyTable"]["movePrevious1"]["Result_val_varchar"]  = table.getStringValue("val_varchar");
               _jsonResult["testModifyTable"]["movePrevious1"]["Result_val_text"]     = table.getStringValue("val_text");
               _jsonResult["testModifyTable"]["movePrevious1"]["Result_val_bigint"]   = table.getStringValue("val_bigint");
               _jsonResult["testModifyTable"]["movePrevious1"]["Result_val_datetime"] = table.getStringValue("val_datetime");
               _jsonResult["testModifyTable"]["movePrevious1"]["Result_val_numeric"]  = table.getStringValue("val_numeric");
            }


            // -------------------------------------------------------
            if (!table.isEof())
            {
               table.movePrevious();

               Logger::logD("[test] note= {}, rawdata= {}, code= {}, val_datetime= {}",
                  table.getStringValue("val_varchar"), table.getStringValue("val_text").substr(0, 20),
                  table.getStringValue("val_bigint"), table.getStringValue("val_datetime"));

               std::string status2;
               if (_sqlTest.option.unicode)
               {
                  /*
                  status2 = (table.getStringValue("val_varchar") ==  u8ToString(u8"કેમ છો")
                             && table.getStringValue("val_text") ==  u8ToString(u8"幫助世界，幫助世界，幫助世界, 幫助世界，幫助世界，幫助世界")
                             && VariantHelper::toString(table.getVariantValue("val_bigint")) == sql::NULLSTR
                             && table.getStringValue("val_datetime") == sql::NULLSTR) ? "PASSED" : "FAILED";
                  */
                  status2 = (table.getStringValue("val_varchar") == sql::NULLSTR
                             && table.getStringValue("val_text") == sql::NULLSTR
                             && std::get<int64_t>(table.getVariantValue("val_bigint")) == 9223372036854775807
                             && util::startsWith(table.getStringValue("val_datetime"), "1999-09-14 19:19:23")) ? "PASSED" : "FAILED";
               }
               else
               {
                  /*
                  status2 = (table.getStringValue("val_varchar") == "HELLO !!!"
                             && table.getStringValue("val_text") == "HELLO WOLRD! A, HELLO WOLRD! B, HELLO WOLRD!"
                             && VariantHelper::toString(table.getVariantValue("val_bigint")) == sql::NULLSTR
                             && table.getStringValue("val_datetime") == sql::NULLSTR) ? "PASSED" : "FAILED";
                  */
                  status2 = (table.getStringValue("val_varchar") == sql::NULLSTR
                             && table.getStringValue("val_text") == sql::NULLSTR
                             && std::get<int64_t>(table.getVariantValue("val_bigint")) == 9223372036854775807
                             && util::startsWith(table.getStringValue("val_datetime"), "1999-09-14 19:19:23")) ? "PASSED" : "FAILED";
               }
               _jsonResult["testModifyTable"]["movePrevious2"]["Status"]              = status2;
               _jsonResult["testModifyTable"]["movePrevious2"]["Result_val_varchar"]  = table.getStringValue("val_varchar");
               _jsonResult["testModifyTable"]["movePrevious2"]["Result_val_text"]     = table.getStringValue("val_text");
               _jsonResult["testModifyTable"]["movePrevious2"]["Result_val_bigint"]   = table.getStringValue("val_bigint");
               _jsonResult["testModifyTable"]["movePrevious2"]["Result_val_datetime"] = table.getStringValue("val_datetime");
               _jsonResult["testModifyTable"]["movePrevious2"]["Result_val_numeric"]  = table.getStringValue("val_numeric");
            }
         }
      }
   }


   void runTestInitSampleData()
   {
      if (_sqlTest.option.runTestInitSampleTables && _sqlTest.testInitSampleTables.initTableSampleData)
      {
         if (_sqlConn.backendType() == sql::BackendType::adodb)
         {
            if (_sqlTest.option.unicode)
            {
               _sqlConn.executeVoid(testado::dropTableSampleDataU);
               _sqlConn.executeVoid(testado::createTableSampleDataU);
               _sqlConn.executeVoid(testado::insertTableSampleDataU);
            }
            else
            {
               _sqlConn.executeVoid(testado::dropTableSampleData);
               _sqlConn.executeVoid(testado::createTableSampleData);
               _sqlConn.executeVoid(testado::insertTableSampleData);
            }
         }
         else if (_sqlConn.backendType() == sql::BackendType::sqlite)
         {
            _sqlConn.executeVoid(testsqlite::dropTableSampleData);
            _sqlConn.executeVoid(testsqlite::createTableSampleData);
            _sqlConn.executeVoid(testsqlite::insertTableSampleData);
         }
         else if (_sqlConn.backendType() == sql::BackendType::pgsql)
         {
            _sqlConn.executeVoid(testpgsql::dropTableSampleData);
            _sqlConn.executeVoid(testpgsql::createTableSampleData);
            _sqlConn.executeVoid(testpgsql::insertTableSampleData);
         }
         else if (_sqlConn.backendType() == sql::BackendType::mysql)
         {
            _sqlConn.executeVoid(testmysql::dropTableSampleData);
            _sqlConn.executeVoid(testmysql::createTableSampleData);
            _sqlConn.executeVoid(testmysql::insertTableSampleData);
         }         
         else if (_sqlConn.backendType() == sql::BackendType::odbc)
         {
            if (_sqlTest.option.dbBackend.isOdbc && _sqlTest.option.dbBackend.backendType == "MSSQL")
            {
               if (_sqlTest.option.unicode)
               {
                  _sqlConn.executeVoid(testodbc_mssql::dropTableSampleDataU);
                  _sqlConn.executeVoid(testodbc_mssql::createTableSampleDataU);

                  if ( osIsWindows() )
                     _sqlConn.executeVoid(testodbc_mssql::insertTableSampleDataU);
                  else
                     insertSampleDataOdbcLinux();
               }
               else
               {
                  _sqlConn.executeVoid(testodbc_mssql::dropTableSampleData);
                  _sqlConn.executeVoid(testodbc_mssql::createTableSampleData);
                  _sqlConn.executeVoid(testodbc_mssql::insertTableSampleData);
               }
            }
            /*
            else if (_sqlTest.option.dbBackend.isOdbc && _sqlTest.option.dbBackend.backendType == "MYSQL")
            {
               _sqlConn.executeVoid(testodbc_mysql::dropTableSampleData);
               _sqlConn.executeVoid(testodbc_mysql::createTableSampleData);

               if ( osIsWindows() )
                  _sqlConn.executeVoid(testodbc_mysql::insertTableSampleData);
               else
                  insertSampleDataOdbcLinux();
            }
            */
            else 
               throw SqlException("Incorrect ODBC backend option", "TestSqlDriver");
         }
         else
            throw SqlException("Unknown BackendType", "TestSqlDriver");
      }
   }


   void runTestInitRawData()
   {
      if (_sqlTest.option.runTestInitSampleTables && _sqlTest.testInitSampleTables.initTableRawData)
      {
         std::string sqlInsert;

         if (_sqlConn.backendType() == sql::BackendType::adodb)
         {
            _sqlConn.executeVoid(testado::dropTableRawData);
            _sqlConn.executeVoid(testado::createTableRawdata);
         }
         else if (_sqlConn.backendType() == sql::BackendType::sqlite)
         {
            _sqlConn.executeVoid(testsqlite::dropTableRawData);
            _sqlConn.executeVoid(testsqlite::createTableRawdata);
         }
         else if (_sqlConn.backendType() == sql::BackendType::pgsql)
         {
            _sqlConn.executeVoid(testpgsql::dropTableRawData);
            _sqlConn.executeVoid(testpgsql::createTableRawdata);
         }
         else if (_sqlConn.backendType() == sql::BackendType::mysql)
         {
            _sqlConn.executeVoid(testmysql::dropTableRawData);
            _sqlConn.executeVoid(testmysql::createTableRawdata);
         }         
         else if (_sqlConn.backendType() == sql::BackendType::odbc)
         {
            if (_sqlTest.option.dbBackend.isOdbc && _sqlTest.option.dbBackend.backendType == "MSSQL")
            {
               _sqlConn.executeVoid(testodbc_mssql::dropTableRawData);
               _sqlConn.executeVoid(testodbc_mssql::createTableRawdata);
            }
            /*
            if (_sqlTest.option.dbBackend.isOdbc && _sqlTest.option.dbBackend.backendType == "MYSQL")
            {
               _sqlConn.executeVoid(testodbc_mysql::dropTableRawData);
               _sqlConn.executeVoid(testodbc_mysql::createTableRawdata);
            }
            */
            else
               throw SqlException("Incorrect ODBC backend option", "TestSqlDriver");
         }
         else {
            throw SqlException("Unknown BackendType", "TestSqlDriver");
         }

         sqlInsert = "INSERT INTO rawdata(note,rawdata,code)  VALUES(:note, :rawdata, :code);";

         long totalRows = _sqlTest.testInitSampleTables.rawDataTotalRow;
         std::string randomStr = util::getRandomString(1024);

         if( _sqlTest.testInitSampleTables.disableLogOnInsert) {
            Logger::disableLogging();
         }

         for (long i = 0; i < totalRows; i++)
         {
            long code = i + 1;
            if (i >= 0 && i < 10)
               code = 10;
            if (i >= 10 && i < 100)
               code = 100;

            std::string note( "NOTE_" + std::to_string(code) );

            sql::SqlQuery query(_sqlConn, sqlInsert);
            query.addParam("note",    sql::DataType::varchar, note);
            query.addParam("rawdata", sql::DataType::varchar, randomStr);
            query.addParam("code",    sql::DataType::integer, code);

            query.executeVoid();

         }

         Logger::enableLogging();
      }
   }


   void insertSampleDataOdbcLinux()
   {
      std::string tableName {"sampledata"};
      if (_sqlTest.option.dbBackend.isOdbc && _sqlTest.option.dbBackend.backendType == "MSSQL")
         tableName = "sampledata_u";


      auto vSampleData = getSampleDataVector();
      int count = 0;
      for (auto &row : vSampleData) // access by reference to avoid copying
      {
         std::string sql =
            tbsfmt::format(R"-(
               INSERT INTO {} (val_bigint, val_varchar, val_text, val_char, val_bool, val_real,
                                      val_double, val_numeric, val_date, val_time, val_datetime, val_binary)
               VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) )-", tableName );
         
         sql::SqlQuery<SqlDriverType> query(_sqlConn, sql);
         query.addParam("bigint",   sql::DataType::bigint,     row.valBigint);
         query.addParam("varchar",  sql::DataType::varchar,    row.valVarchar);
         query.addParam("text",     sql::DataType::text,       row.valText);
         query.addParam("char",     sql::DataType::character,  row.valChar);
         query.addParam("bool",     sql::DataType::boolean,    row.valBool);
         query.addParam("float",    sql::DataType::float4,     row.valReal);
         query.addParam("double",   sql::DataType::float8,     row.valDouble);
         // use std::string for sql's numeric type, no need to include size,direction and decimal digit
         query.addParam("numeric",  sql::DataType::numeric,    row.valNumeric);
         // use std::string for sql's date,  format is yyyy-mm-dd
         query.addParam("date",     sql::DataType::date,       row.valDate);
         // use std::string for sql's time,  format is hh:mm:ss
         query.addParam("time",     sql::DataType::time,       row.valTime);
         // use std::string for sql's timestamp,  format is yyyy-mm-dd hh:mm:ss
         query.addParam("datetime", sql::DataType::timestamp,  row.valDatetime);
         // use Hexadecimal encoded binary string for sql's varbinary, and specify raw data's size
         query.addParam("binary",   sql::DataType::varbinary,  row.valBinary,  static_cast<long>(row.valBinary.length()/2) );

         int rowsAffected = query.execute();
         count ++;
      }
   }


   bool osIsWindows()
   {
#if defined(_WIN32) || defined(WIN32)
      return true;
#else
      return false;
#endif
   }

};

} //namespace test
} //namespace tbs