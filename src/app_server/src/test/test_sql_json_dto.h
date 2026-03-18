#pragma once

#include <string>
#include <tobasa/json.h>

namespace tbs {
namespace test {

struct TableOption
{
   int         pageSize;
   int         pagePos;
   std::string orderBy;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TableOption, pageSize, pagePos, orderBy)


struct SelectQueryA
{
   bool        run;
   std::string cmd;
   long        param1;
   long long   param2;
   std::string param3;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SelectQueryA, run, cmd, param1, param2, param3)


struct SelectQueryB
{
   bool        run;
   std::string cmd;
   long        param1;
   std::string param2;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SelectQueryB, run, cmd, param1, param2)


struct InsertQuery
{
   std::string cmd;
   std::string param1;
   std::string param2;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InsertQuery, cmd, param1, param2)


struct InsertQueryB
{
   bool        run;
   std::string cmd;
   long long   paramBigInt;
   std::string paramChar;
   bool        paramBool;
   float       paramFloat;
   double      paramDouble;
   std::string paramNumeric;
   std::string paramDate;
   std::string paramTime;
   std::string paramDateTime;
   std::string paramBinary;
   std::string paramBinary1;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InsertQueryB, run, cmd, paramBigInt, paramChar, paramBool, paramFloat, paramDouble, paramNumeric, paramDate, paramTime, paramDateTime, paramBinary, paramBinary1)


// -------------------------------------------------------
struct DirectMode
{
   std::string insertCmd;
   std::string selectCmd;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DirectMode, insertCmd, selectCmd)

struct PreparedMode
{
   std::string insertCmd;
   std::string selectCmd;
   std::string paramInsertVarchar;
   std::string paramInsertText;
   long long   paramInsertBigint;
   std::string paramInsertDate;
   std::string paramSelectVarchar;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PreparedMode, insertCmd, selectCmd, paramInsertVarchar, paramInsertText, paramInsertBigint, paramInsertDate, paramSelectVarchar)

struct TestDirectAndPrepared
{
   bool         useDirectMode;
   DirectMode   directMode;
   PreparedMode preparedMode;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TestDirectAndPrepared, useDirectMode, directMode, preparedMode)
// -------------------------------------------------------

struct TestQueryWithParameter
{
   SelectQueryA selectQueryA;
   SelectQueryB selectQueryB;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TestQueryWithParameter, selectQueryA, selectQueryB)


struct TestQuerySelectInsert
{
   bool         useParamFromJson;
   SelectQueryB selectQueryB;
   InsertQuery  insertQuery;
   InsertQueryB insertQueryB;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TestQuerySelectInsert, useParamFromJson, selectQueryB, insertQuery, insertQueryB)


struct TestSqlResult
{
   std::string  tableName;
   bool         optionCache;
   bool         optionTable;
   bool         useParameter;
   SelectQueryA selectQueryA;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TestSqlResult, tableName, optionCache, optionTable, useParameter, selectQueryA)


struct TestOpenTable
{
   std::string tableName;
   std::string customSelect;
   TableOption option;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TestOpenTable, tableName, customSelect, option)


struct TestModifyTable
{
   std::string tableName;
   long        paramId;
   std::string paramVarChar;
   std::string paramText;
   long long   paramBigInt;
   std::string paramChar;
   std::string paramDate;
   std::string paramTime;
   std::string paramDateTime;
   float       paramFloat;
   double      paramDouble;
   std::string paramNumeric;
   bool        paramBool;

   long        paramSelectId;
   TableOption option;
   bool        insertWithNativeVariant;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TestModifyTable, tableName, paramId, paramVarChar, paramText, paramBigInt, paramChar, paramDate, paramTime, paramDateTime, paramFloat, paramDouble, paramNumeric, paramBool, paramSelectId, option, insertWithNativeVariant)


struct TestInitSampleTables
{
   bool initTableSampleData;
   bool initTableRawData;
   long rawDataTotalRow;
   bool disableLogOnInsert;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TestInitSampleTables, initTableSampleData, initTableRawData, rawDataTotalRow, disableLogOnInsert)


struct DbBackend
{
   bool isOdbc;
   std::string backendType;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DbBackend, isOdbc, backendType)

struct TestOption
{
   DbBackend dbBackend;
   bool unicode;
   bool runTestInitSampleTables;
   bool runTestDirectAndPrepared;
   bool runTestQueryWithParameter;
   bool runTestQuerySelectInsert;
   bool runTestSqlResult;
   bool runTestOpenTable;
   bool runTestModifyTable;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TestOption, dbBackend, unicode, runTestInitSampleTables, runTestDirectAndPrepared, runTestQueryWithParameter, runTestQuerySelectInsert, runTestSqlResult, runTestOpenTable, runTestModifyTable)


// Our full Json test definitions
struct SqlTest
{
   std::string               connString;
   TestOption                option;
   TestInitSampleTables      testInitSampleTables;
   TestDirectAndPrepared     testDirectAndPrepared;
   TestQueryWithParameter    testQueryWithParameter;
   TestQuerySelectInsert     testQuerySelectInsert;
   TestSqlResult             testSqlResult;
   TestOpenTable             testOpenTable;
   TestModifyTable           testModifyTable;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SqlTest, connString, option, testInitSampleTables, testDirectAndPrepared, testQueryWithParameter, testQuerySelectInsert, testSqlResult, testOpenTable, testModifyTable)


} // namespace test
} // namespace tbs
