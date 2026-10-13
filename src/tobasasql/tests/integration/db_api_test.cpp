
/*
   NOTE: Save this file as UTF-8 with BOM so the Unicode text in this test is preserved correctly.
*/

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include <tobasa/bin_encode.h>
#include <tobasa/common.h>
#include <tobasa/datetime.h>
#include <tobasa/util_string.h>
#include "tobasa_test_support/environment.h"
#include "tobasa_test_support/util.h"
#include "tobasa_test_support/util_u8.h"
#include "tobasasql/db_api.h"

#if defined(TOBASA_DB_TEST_SQLITE)
   #include "sqlite/generic_api_test.h"
   #define DB_GENERIC_TEST_NAMESPACE test_sqlite
   #define DB_GENERIC_TEST_SUITE SqliteDbGenericAPIIntegrationTest
   #define DB_GENERIC_TEST_ENV "TOBASA_SQLITE_TEST_CONNECTION"
   #define DB_GENERIC_TEST_BACKEND tbs::sql::BackendType::sqlite
#elif defined(TOBASA_DB_TEST_PGSQL)
   #include "postgres/generic_api_test.h"
   #define DB_GENERIC_TEST_NAMESPACE test_pgsql
   #define DB_GENERIC_TEST_SUITE PgsqlDbGenericAPIIntegrationTest
   #define DB_GENERIC_TEST_ENV "TOBASA_PGSQL_TEST_CONNECTION"
   #define DB_GENERIC_TEST_BACKEND tbs::sql::BackendType::pgsql
#elif defined(TOBASA_DB_TEST_MYSQL)
   #include "mysql/generic_api_test.h"
   #define DB_GENERIC_TEST_NAMESPACE test_mysql
   #define DB_GENERIC_TEST_SUITE MysqlDbGenericAPIIntegrationTest
   #define DB_GENERIC_TEST_ENV "TOBASA_MYSQL_TEST_CONNECTION"
   #define DB_GENERIC_TEST_BACKEND tbs::sql::BackendType::mysql
#elif defined(TOBASA_DB_TEST_ODBC)
   #include "odbc/generic_api_test.h"
   #define DB_GENERIC_TEST_NAMESPACE test_odbc
   #define DB_GENERIC_TEST_SUITE OdbcDbGenericAPIIntegrationTest
   #define DB_GENERIC_TEST_ENV "TOBASA_ODBC_TEST_CONNECTION"
   #define DB_GENERIC_TEST_BACKEND tbs::sql::BackendType::odbc
#elif defined(TOBASA_DB_TEST_ADODB)
   #include "adodb/generic_api_test.h"
   #include "tobasasql/com_initializer.h"
   #define DB_GENERIC_TEST_NAMESPACE test_adodb
   #define DB_GENERIC_TEST_SUITE AdodbDbGenericAPIIntegrationTest
   #define DB_GENERIC_TEST_ENV "TOBASA_ADODB_TEST_CONNECTION_SQLNCLI"
   #define DB_GENERIC_TEST_BACKEND tbs::sql::BackendType::adodb
#endif

#if defined(TOBASA_DB_TEST_MYSQL) || defined(TOBASA_DB_TEST_ODBC) || defined(TOBASA_DB_TEST_ADODB)
   #define DB_GENERIC_TEST_COLUMN_COUNT 16
#else
   #define DB_GENERIC_TEST_COLUMN_COUNT 14
#endif

namespace test_db_generic_api {

using namespace tbs;
using namespace tbs::sql;
using namespace tbs::test_support;
using namespace DB_GENERIC_TEST_NAMESPACE;

std::string loadConnectionString()
{
   return loadEnvironmentValue(DB_GENERIC_TEST_ENV,
      std::string(TOBASA_PROJECT_SOURCE_DIR) + "/.env");
}

std::string normalizeTimeX(const std::string& value)
{
   if (value.size() == 5)
      return normalizeTime(value + ":00");
   return normalizeTime(value);
}

std::string normalizeDateTimeX(const std::string& value)
{
   if (value.size() == 16)
      return normalizeDateTime(value + ":00");
   return normalizeDateTime(value);
}

DbConnectionOptions makeConnectionOptions(const std::string& connectionString)
{
   DbConnectionOptions options;
   options.backend                    = DB_GENERIC_TEST_BACKEND;
   options.connectionString           = connectionString;
   options.connectionStringIsComplete = true;
   return options;
}

TEST(DB_GENERIC_TEST_SUITE, ConnectExecuteAndExecuteScalar)
{
   const std::string connectionString = loadConnectionString();

   if (connectionString.empty())
      GTEST_SKIP() << "Set " << DB_GENERIC_TEST_ENV << " or add it to .env";

#if defined(TOBASA_DB_TEST_ADODB)
   ComInitializer comInitializer(true);
#endif

   DbConnection conn(makeConnectionOptions(connectionString));
   EXPECT_EQ( conn.execute("SELECT 1"), 0);
   EXPECT_EQ( conn.executeScalar("SELECT 1"), "1");

   auto cmd = conn.createCommand("SELECT 1");
   EXPECT_EQ( cmd.execute(), 0);
   cmd.reset();
   EXPECT_EQ( cmd.executeScalar(), "1");
   cmd.reset();
   EXPECT_EQ( cmd.execute(), 0);
   cmd.reset();
   EXPECT_EQ( cmd.executeScalar(), "1");

   auto res = conn.createCommand("SELECT 1").executeQuery();
   ASSERT_TRUE( res.isValid());
   EXPECT_EQ( res.getString(0), "1");
}


TEST(DB_GENERIC_TEST_SUITE, CRUDAndDataRetrieval)
{
   // We need this for Tobasa DateTime objects or SQL date/time conversion:
   ASSERT_TRUE(tbs::DateTime::initTimezoneData());

   const std::string connectionString = loadConnectionString();

   if (connectionString.empty())
      GTEST_SKIP() << "Set " << DB_GENERIC_TEST_ENV << " or add it to .env";

#if defined(TOBASA_DB_TEST_ADODB)
   ComInitializer comInitializer(true);
#endif

   DbConnection connection(makeConnectionOptions(connectionString));

   // ------------------------------------------------------------------------------
   // Connect, initialize the database table, and verify its structure.
   // NOTE: we are using SQL Text directly
   // ------------------------------------------------------------------------------
   EXPECT_EQ( connection.execute(dropTableSampleData), 0);
   EXPECT_EQ( connection.execute(createTableSampleData), 0);
   EXPECT_EQ( connection.execute(insertTableSampleData), 4);

   // Retrieve the result via the facade result API
   auto result = connection.createCommand(
      "SELECT * FROM sampledata ORDER BY id ASC").executeQuery();
   ASSERT_EQ( result.totalRows(), 4);
   ASSERT_EQ( result.totalColumns(), DB_GENERIC_TEST_COLUMN_COUNT);

   // Retrieve the result via the facade command API
   auto command = connection.createCommand("SELECT * FROM sampledata ORDER BY id ASC");
   auto pRes = command.executeQuery();
   ASSERT_TRUE( pRes.isValid());
   ASSERT_EQ( pRes.totalRows(), 4);
   ASSERT_EQ( pRes.totalColumns(), DB_GENERIC_TEST_COLUMN_COUNT);

   // ------------------------------------------------------------------------------
   // Move to the last row and validate the field values.
   // ------------------------------------------------------------------------------
   pRes.moveLast();

   EXPECT_EQ( pRes.getString("id"),           "4");
   EXPECT_EQ( pRes.getString("val_varchar"),  u8"Привет 4"_asChar);
   EXPECT_EQ( pRes.getString("val_text"),     u8"Γεια σου κόσμε! 4"_asChar);
   EXPECT_EQ( pRes.getString("val_bigint"),   "9223372036854775807");
#if defined(TOBASA_DB_TEST_PGSQL) || defined(TOBASA_DB_TEST_ODBC) || defined(TOBASA_DB_TEST_ADODB)
   EXPECT_EQ( pRes.getString("val_char"),     u8"ППППП04             "_asChar);
#else
   EXPECT_EQ( pRes.getString("val_char"),     u8"ППППП04"_asChar);
#endif
   EXPECT_EQ( normalizeDate(     pRes.getString("val_date")),     "2002-04-18");
   EXPECT_EQ( normalizeTimeX(    pRes.getString("val_time")),     "04:40:00");
   EXPECT_EQ( normalizeDateTimeX(pRes.getString("val_datetime")), "2004-04-14 15:21:00");

   EXPECT_EQ( pRes.getString("val_real"),     "44.123455");
   EXPECT_EQ( pRes.getString("val_double"),   "44.123456789012344");
   EXPECT_EQ( pRes.getString("val_numeric"),  "433444666774.45");
   
#if defined(TOBASA_DB_TEST_MYSQL)
   EXPECT_EQ( pRes.getString("val_bool"), "0");
#else
   EXPECT_EQ( pRes.getString("val_bool"), "false");
#endif
   
   EXPECT_EQ( pRes.getString("val_binary1"),  "");
   EXPECT_EQ( util::toUpper(pRes.getString("val_binary")), "342E20544F42415341");

   // ------------------------------------------------------------------------------
   // Read the third row values using typed accessors where appropriate.
   // ------------------------------------------------------------------------------
   pRes.locate(2);

   EXPECT_EQ( pRes.getInt64(0),         3);
   EXPECT_EQ( pRes.getString(1),        u8ToString(u8"Ողջույն 3"));
   EXPECT_EQ( pRes.getString(2),        u8ToString(u8"Բարև աշխարհ! QQQ3"));
   EXPECT_EQ( pRes.getInt64(3),         9223372036854775806LL);
#if defined(TOBASA_DB_TEST_PGSQL) || defined(TOBASA_DB_TEST_ODBC) || defined(TOBASA_DB_TEST_ADODB)
   EXPECT_EQ( pRes.getString(4),        u8"ППППП03             "_asChar);
#else
   EXPECT_EQ( pRes.getString(4),        u8"ППППП03"_asChar);
#endif
   EXPECT_EQ( normalizeDate(     pRes.getString(5)), "2001-04-18");
   EXPECT_EQ( normalizeTimeX(    pRes.getString(6)), "03:40:00");
   EXPECT_EQ( normalizeDateTimeX(pRes.getString(7)), "2003-03-14 15:21:00");

   EXPECT_FLOAT_EQ( pRes.getFloat(8),   33.1234550f);
   EXPECT_DOUBLE_EQ( pRes.getDouble(9), 34.123456789012344);
   EXPECT_EQ( pRes.getString(10),       "334446666773.45");
   EXPECT_TRUE( pRes.getBool(11));
   EXPECT_EQ( pRes.getString(13),       "");
   EXPECT_EQ( util::toUpper(pRes.getString(12)),   "332E20544F42415341");

   // ------------------------------------------------------------------------------
   // Back to the first row and decode the binary value from column 12.
   // ------------------------------------------------------------------------------
   pRes.moveFirst();

   const std::vector<std::string> expectedAscii = { "1. TOBASA", "2. TOBASA", "3. TOBASA", "4. TOBASA" };

   for (int row = 0; row < pRes.totalRows(); ++row)
   {
      const std::string hexValue = pRes.getString(12);
      std::vector<tbs::byte_t> decoded(hexValue.size() / 2);
      tbs::conv::hexDecode(hexValue, decoded.data());
      const std::string asciiValue(reinterpret_cast<const char*>(decoded.data()), decoded.size());
      EXPECT_EQ(asciiValue, expectedAscii[static_cast<size_t>(row)]);

      if (row + 1 < pRes.totalRows())
         pRes.moveNext();
   }


   // ------------------------------------------------------------------------------
   // INSERT Data using data from getSampleDataVector()
   // ------------------------------------------------------------------------------
#if defined(TOBASA_DB_TEST_PGSQL)
   const std::string sql = R"-(
      INSERT INTO sampledata (val_bigint, val_varchar, val_text, val_char, val_bool, val_real,
                              val_double, val_numeric, val_date, val_time, val_datetime, val_binary)
      VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12)
   )-";
#else
   const std::string sql = R"-(
      INSERT INTO sampledata (val_bigint, val_varchar, val_text, val_char, val_bool, val_real,
                              val_double, val_numeric, val_date, val_time, val_datetime, val_binary)
      VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
   )-";
#endif

   auto cmdInsert = connection.createCommand(sql, ParameterStyle::native);

   auto vSampleData = getSampleDataVector();
   int count = 0;
   for (auto &row : vSampleData) // access by reference to avoid copying
   {
      cmdInsert.addParam("bigint",   sql::DataType::bigint,     row.valBigint);
      cmdInsert.addParam("varchar",  sql::DataType::varchar,    row.valVarchar);
      cmdInsert.addParam("text",     sql::DataType::text,       row.valText);
      cmdInsert.addParam("char",     sql::DataType::character,  row.valChar);
      cmdInsert.addParam("bool",     sql::DataType::boolean,    row.valBool);
      cmdInsert.addParam("real",     sql::DataType::float4,     row.valReal);
      cmdInsert.addParam("double",   sql::DataType::float8,     row.valDouble);
      // use std::string for sql numeric type, no need to include size,direction and decimal digit
      cmdInsert.addParam("numeric",  sql::DataType::numeric,    row.valNumeric);
      // use std::string for sql date, format is yyyy-mm-dd
      cmdInsert.addParam("date",     sql::DataType::date,       row.valDate);
      // use std::string for sql time, format is hh:mm:ss
      cmdInsert.addParam("time",     sql::DataType::time,       row.valTime);
      // use std::string for sql timestamp, format is yyyy-mm-dd hh:mm:ss
      cmdInsert.addParam("datetime", sql::DataType::timestamp,  row.valDatetime);
      // use Hexadecimal encoded binary string for sql varbinary, and specify raw data's size
      cmdInsert.addParam("binary",   sql::DataType::varbinary,  row.valBinary, static_cast<uint64_t>(row.valBinary.length() / 2));

      const int affectedRows = cmdInsert.execute();
      EXPECT_EQ(affectedRows, 1);

      // reset the command for the next execute()
      cmdInsert.reset();
      ++count;
   }

   // after reset, the last affected row count should be cleared.
   EXPECT_EQ(cmdInsert.affectedRows(), -1);

   auto result2 = connection.createCommand(
      "SELECT * FROM sampledata ORDER BY id ASC").executeQuery();
   ASSERT_EQ(result2.totalRows(), 8);
   ASSERT_EQ(result2.totalColumns(), DB_GENERIC_TEST_COLUMN_COUNT);

   // ------------------------------------------------------------------------------
   // INSERT Data, using first two rows data from result2
   // ------------------------------------------------------------------------------
   result2.moveFirst();
   for (int i = 0; i < 2; i++) // access by reference to avoid copying
   {
      cmdInsert.addParam("bigint",   sql::DataType::bigint,     result2.getInt64("val_bigint"));
      cmdInsert.addParam("varchar",  sql::DataType::varchar,    result2.getString("val_varchar"));
      cmdInsert.addParam("text",     sql::DataType::text,       result2.getString("val_text"));
      cmdInsert.addParam("char",     sql::DataType::character,  result2.getString("val_char"));
      cmdInsert.addParam("bool",     sql::DataType::boolean,    result2.getBool("val_bool"));
      cmdInsert.addParam("real",     sql::DataType::float4,     result2.getFloat("val_real"));
      cmdInsert.addParam("double",   sql::DataType::float8,     result2.getDouble("val_double"));
      // use std::string for sql numeric type, no need to include size,direction and decimal digit
      cmdInsert.addParam("numeric",  sql::DataType::numeric,    result2.getString("val_numeric"));
      // use std::string for sql date, format is yyyy-mm-dd
      cmdInsert.addParam("date",     sql::DataType::date,       normalizeDate(result2.getString("val_date")));
      // use std::string for sql time, format is hh:mm:ss
      cmdInsert.addParam("time",     sql::DataType::time,       normalizeTimeX(result2.getString("val_time")));
      // use std::string for sql timestamp, format is yyyy-mm-dd hh:mm:ss
      cmdInsert.addParam("datetime", sql::DataType::timestamp,  normalizeDateTimeX(result2.getString("val_datetime")));
      // use Hexadecimal encoded binary string for sql varbinary, and specify raw data's size
      auto hexData = result2.getString("val_binary");
      cmdInsert.addParam("binary",   sql::DataType::varbinary,  hexData, static_cast<uint64_t>(hexData.length() / 2));

      ASSERT_EQ(cmdInsert.execute(), 1);

      // reset the command for the next execute()
      cmdInsert.reset();

      if (i + 1 < 2)
         result2.moveNext();
   }


   // ------------------------------------------------------------------------------
   // Retrieve the table, move to the last row and validate the field values with result2 6th rows(idx 5).
   // ------------------------------------------------------------------------------
   result2.locate(5);
   auto result3 = connection.createCommand(
      "SELECT * FROM sampledata ORDER BY id ASC").executeQuery();
   ASSERT_EQ( result3.totalRows(), 10);
   ASSERT_EQ( result3.totalColumns(), DB_GENERIC_TEST_COLUMN_COUNT);
   result3.moveLast();

   EXPECT_EQ( result3.getString("id"),           "10");
   EXPECT_EQ( result3.getString("val_varchar"),  result2.getString("val_varchar"));
   EXPECT_EQ( result3.getString("val_text"),     result2.getString("val_text"));
   EXPECT_EQ( result3.getString("val_bigint"),   "2222222222222222222");
#if defined(TOBASA_DB_TEST_PGSQL) || defined(TOBASA_DB_TEST_ODBC) || defined(TOBASA_DB_TEST_ADODB)
   EXPECT_EQ( result3.getString("val_char"),     u8"ППППП02             "_asChar);
#else
   EXPECT_EQ( result3.getString("val_char"),     u8"ППППП02"_asChar);
#endif
   EXPECT_EQ( normalizeDate(     result3.getString("val_date")),     "2000-07-21");
   EXPECT_EQ( normalizeTimeX(    result3.getString("val_time")),     "12:20:00");
   EXPECT_EQ( normalizeDateTimeX(result3.getString("val_datetime")), "2002-02-11 19:55:00");
   EXPECT_EQ( result3.getString("val_real"),     "22.123457");
   EXPECT_EQ( result3.getString("val_double"),   "24.123456789012344");
   EXPECT_EQ( result3.getString("val_numeric"),  "234446666772.45");

#if defined(TOBASA_DB_TEST_MYSQL)
   EXPECT_EQ( result3.getString("val_bool"), "0");
#else
   EXPECT_EQ( result3.getString("val_bool"), "false");
#endif

   EXPECT_EQ( result3.getString("val_binary1"), "");
   EXPECT_EQ( util::toUpper(result3.getString("val_binary")), "322E20544F42415341");
}

} // namespace test_db_generic_api
