#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "tobasa_test_support/environment.h"
#include "tobasa_test_support/util_u8.h"

#include "tobasasql/sql_driver.h"
#include "tobasasql/sql_connection.h"
#include "tobasasql/sql_result.h"
#include "tobasasql/sql_query.h"

#include "generic_api_test.h"

/*
   NOTE: Save this file as UTF-8 with BOM so the Unicode text in this test is preserved correctly.
*/

namespace test_pgsql {

using namespace tbs;
using namespace tbs::sql;
using namespace tbs::test_support;

using SqlConnection = sql::SqlConnection<sql::PgsqlDriver>;
using SqlResult     = sql::SqlResult<sql::PgsqlDriver>;
using SqlQuery      = sql::SqlQuery<sql::PgsqlDriver>;
using SqlCommand    = sql::SqlQuery<sql::PgsqlDriver>;

TEST(PgsqlGenericAPIIntegrationTest, ConnectExecuteAndExecuteScalar)
{
   const std::string connectionString = test_support::loadEnvironmentValue(
      "TOBASA_PGSQL_TEST_CONNECTION", std::string(TOBASA_PROJECT_SOURCE_DIR) + "/.env");

   if (connectionString.empty())
      GTEST_SKIP() << "Set TOBASA_PGSQL_TEST_CONNECTION or add it to .env";

   SqlConnection conn;
   ASSERT_TRUE(  conn.connect(connectionString)        );
   EXPECT_EQ(    conn.execute("SELECT 1"), 0           );
   EXPECT_EQ(    conn.executeScalar("SELECT 1"), "1"   );

   SqlCommand    cmd(conn);

   ASSERT_TRUE(  cmd.query("SELECT 1", {}) );
   EXPECT_EQ(    cmd.execute(), 0          );
   //            cmd.reset();

   EXPECT_EQ(    cmd.executeScalar(), "1"  );
   //            cmd.reset();

   ASSERT_TRUE(  cmd.prepare("SELECT 1")   );
   
   EXPECT_EQ(    cmd.execute(), 0);
   //            cmd.reset();

   EXPECT_EQ(    cmd.executeScalar(), "1"  );
   //            cmd.reset();

   SqlResult     res(conn);
   ASSERT_TRUE(  res.runQuery("SELECT 1")    );
   EXPECT_EQ(    res.getStringValue(0), "1"  );
}


TEST(PgsqlGenericAPIIntegrationTest, CRUDAndDataRetrieval)
{
   const std::string connectionString = test_support::loadEnvironmentValue(
      "TOBASA_PGSQL_TEST_CONNECTION", std::string(TOBASA_PROJECT_SOURCE_DIR) + "/.env");

   if (connectionString.empty())
      GTEST_SKIP() << "Set TOBASA_PGSQL_TEST_CONNECTION or add it to .env";

   SqlConnection connection;

   // ------------------------------------------------------------------------------
   // Connect, initialize the database table, and verify its structure.
   // NOTE: we are using SQL Text directly
   // ------------------------------------------------------------------------------
   ASSERT_TRUE(connection.connect(connectionString));
   EXPECT_EQ(  connection.execute(dropTableSampleData),    0);
   EXPECT_EQ(  connection.execute(createTableSampleData),  0);
   EXPECT_EQ(  connection.execute(insertTableSampleData),  4);

   // Retrive the result via SqlResult
   SqlResult result(connection);
   result.runQuery("SELECT * FROM sampledata ORDER BY id ASC");
   ASSERT_EQ( result.totalRows(),       4);
   ASSERT_EQ( result.totalColumns(),    14);
   
   // Retrive the result via SqlQuery
   SqlQuery command(connection);
   command.query("SELECT * FROM sampledata ORDER BY id ASC");
   auto pRes = command.executeResult();
   ASSERT_NE(pRes, nullptr);
   ASSERT_EQ(pRes->totalRows(), 4);
   ASSERT_EQ(pRes->totalColumns(), 14);

   // ------------------------------------------------------------------------------
   // Move to the last row and validate the field values.
   // ------------------------------------------------------------------------------
   pRes->moveLast();

   EXPECT_EQ(pRes->getStringValue("id"),           "4");
   EXPECT_EQ(pRes->getStringValue("val_varchar"),  u8"Привет 4"_asChar   );
   EXPECT_EQ(pRes->getStringValue("val_text"),     u8"Γεια σου κόσμε! 4"_asChar  );
   EXPECT_EQ(pRes->getStringValue("val_bigint"),   "9223372036854775807");
   EXPECT_EQ(pRes->getStringValue("val_char"),     u8"ППППП04             "_asChar ); // DB CHAR, Note the spaces
   EXPECT_EQ(pRes->getStringValue("val_date"),     "2002-04-18");
   EXPECT_EQ(pRes->getStringValue("val_time"),     "04:40:00");               // Postgres added seconds
   EXPECT_EQ(pRes->getStringValue("val_datetime"), "2004-04-14 15:21:00");    // Postgres added seconds
   EXPECT_EQ(pRes->getStringValue("val_real"),     "44.123455");              // DB REAL, rounded by Postgres
   EXPECT_EQ(pRes->getStringValue("val_double"),   "44.123456789012344");     // DB DOUBLE, rounded by Postgres
   EXPECT_EQ(pRes->getStringValue("val_numeric"),  "433444666774.45");        // 
   EXPECT_EQ(pRes->getStringValue("val_bool"),     "false");                  // libpq received as "f", tobasasql normalized it as "false"
   EXPECT_EQ(pRes->getStringValue("val_binary1"),   "");
   EXPECT_EQ(util::toUpper(pRes->getStringValue("val_binary")), "342E20544F42415341");

   // ------------------------------------------------------------------------------
   // Read the third row values using typed accessors where appropriate.
   // ------------------------------------------------------------------------------
   pRes->locate(2);

   EXPECT_EQ( pRes->getLongValue(0),            3L);
   EXPECT_EQ( pRes->getStringValue(1),          u8ToString(u8"Ողջույն 3"));
   EXPECT_EQ( pRes->getStringValue(2),          u8ToString(u8"Բարև աշխարհ! QQQ3"));
   EXPECT_EQ( pRes->getLongLongValue(3),        9223372036854775806LL);
   EXPECT_EQ( pRes->getStringValue(4),          u8"ППППП03             "_asChar);
   EXPECT_EQ( pRes->getStringValue(5),          "2001-04-18");
   EXPECT_EQ( pRes->getStringValue(6),          "03:40:00");
   EXPECT_EQ( pRes->getStringValue(7),          "2003-03-14 15:21:00");
   EXPECT_FLOAT_EQ(  pRes->getFloatValue(8),    33.1234550f);
   EXPECT_DOUBLE_EQ( pRes->getDoubleValue(9),   34.1234567890123456);
   EXPECT_EQ(        pRes->getStringValue(10),  "334446666773.45");
   EXPECT_TRUE(      pRes->getBoolValue(11));
   EXPECT_EQ(        pRes->getStringValue(13),  "");
   EXPECT_EQ( util::toUpper(pRes->getStringValue(12)), "332E20544F42415341");

   // ------------------------------------------------------------------------------
   // Back to the first row and decode the bytea value from column 12.
   // ------------------------------------------------------------------------------
   pRes->moveFirst();

   const std::vector<std::string> expectedAscii = {
      "1. TOBASA", "2. TOBASA", "3. TOBASA", "4. TOBASA"
   };

   for (int row = 0; row < pRes->totalRows(); ++row)
   {
      const std::string hexValue = pRes->getStringValue(12);
      std::vector<tbs::byte_t> decoded(hexValue.size() / 2);
      tbs::conv::hexDecode(hexValue, decoded.data());
      const std::string asciiValue(reinterpret_cast<const char*>(decoded.data()), decoded.size());
      EXPECT_EQ(asciiValue, expectedAscii[static_cast<size_t>(row)]);

      if (row + 1 < pRes->totalRows())
         pRes->moveNext();
   }


   // ------------------------------------------------------------------------------
   // INSERT Data using native PostgreSQL positional parameters.
   // Data from getSampleDataVector()
   // ------------------------------------------------------------------------------
   std::string sql =
      tbsfmt::format(R"-(
         INSERT INTO sampledata (val_bigint, val_varchar, val_text, val_char, val_bool, val_real,
                                 val_double, val_numeric, val_date, val_time, val_datetime, val_binary)
         VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12)
      )-" );

   SqlQuery cmdInsert(connection, sql, ParameterStyle::native);

   auto vSampleData = getSampleDataVector();
   int count = 0;
   for (auto &row : vSampleData) // access by reference to avoid copying
   {
      cmdInsert.addParam("bigint",   sql::DataType::bigint,     row.valBigint);
      cmdInsert.addParam("varchar",  sql::DataType::varchar,    row.valVarchar);
      cmdInsert.addParam("text",     sql::DataType::text,       row.valText);
      cmdInsert.addParam("char",     sql::DataType::character,  row.valChar);
      cmdInsert.addParam("bool",     sql::DataType::boolean,    row.valBool);
      cmdInsert.addParam("float",    sql::DataType::float4,     row.valReal);   // float  value rounded by the compiler
      cmdInsert.addParam("double",   sql::DataType::float8,     row.valDouble); // double value rounded by the compiler
      // use std::string for sql numeric type, no need to include size,direction and decimal digit
      cmdInsert.addParam("numeric",  sql::DataType::numeric,    row.valNumeric);
      // use std::string for sql date,  format is yyyy-mm-dd
      cmdInsert.addParam("date",     sql::DataType::date,       row.valDate);
      // use std::string for sql time,  format is hh:mm:ss
      cmdInsert.addParam("time",     sql::DataType::time,       row.valTime);
      // use std::string for sql timestamp,  format is yyyy-mm-dd hh:mm:ss
      cmdInsert.addParam("datetime", sql::DataType::timestamp,  row.valDatetime);
      // use Hexadecimal encoded binary string for sql varbinary, and specify raw data's size
      cmdInsert.addParam("binary",   sql::DataType::varbinary,  row.valBinary,  static_cast<long>(row.valBinary.length()/2) );

      const int affectedRows = cmdInsert.execute();
      EXPECT_EQ(affectedRows, 1);

      // reset the command for the next execute()
      cmdInsert.reset();
      ++count;
   }

   // after reset, the last affected row count should be cleared.
   EXPECT_EQ(cmdInsert.affectedRows(), -1);

   SqlResult result2(connection);
   ASSERT_TRUE(result2.runQuery("SELECT * FROM sampledata ORDER BY id ASC"));
   ASSERT_EQ(result2.totalRows(), 8);
   ASSERT_EQ(result2.totalColumns(), 14);


   // ------------------------------------------------------------------------------
   // INSERT Data, using first two rows data from result2
   // ------------------------------------------------------------------------------
   result2.moveFirst();
   for (int i=0; i < 2; i++) // access by reference to avoid copying
   {
      cmdInsert.addParam("bigint",   sql::DataType::bigint,     result2.getLongLongValue("val_bigint") );
      cmdInsert.addParam("varchar",  sql::DataType::varchar,    result2.getStringValue("val_varchar"));
      cmdInsert.addParam("text",     sql::DataType::text,       result2.getStringValue("val_text"));
      cmdInsert.addParam("char",     sql::DataType::character,  result2.getStringValue("val_char"));
      cmdInsert.addParam("bool",     sql::DataType::boolean,    result2.getBoolValue("val_bool"));
      auto floatVal = result2.getFloatValue("val_real");
      cmdInsert.addParam("float",    sql::DataType::float4,     result2.getFloatValue("val_real"));
      auto doubleVal = result2.getDoubleValue("val_double");
      cmdInsert.addParam("double",   sql::DataType::float8,     result2.getDoubleValue("val_double"));
      // use std::string for sql numeric type, no need to include size,direction and decimal digit
      cmdInsert.addParam("numeric",  sql::DataType::numeric,    result2.getStringValue("val_numeric"));
      // use std::string for sql date,  format is yyyy-mm-dd
      cmdInsert.addParam("date",     sql::DataType::date,       result2.getStringValue("val_date"));
      // use std::string for sql time,  format is hh:mm:ss
      cmdInsert.addParam("time",     sql::DataType::time,       result2.getStringValue("val_time"));
      // use std::string for sql timestamp,  format is yyyy-mm-dd hh:mm:ss
      cmdInsert.addParam("datetime", sql::DataType::timestamp,  result2.getStringValue("val_datetime"));
      
      // use Hexadecimal encoded binary string for sql varbinary, and specify raw data's size
      auto hexData = result2.getStringValue("val_binary");
      cmdInsert.addParam("binary",   sql::DataType::varbinary,  hexData,  static_cast<long>(hexData.length()/2) );

      ASSERT_EQ( cmdInsert.execute(), 1);
      // reset the command for the next execute()
      cmdInsert.reset();

      result2.moveNext();
   }


   // ------------------------------------------------------------------------------
   // Retrieve the table, move to the last row and validate the field values with result2 6th rows(idx 5).
   // ------------------------------------------------------------------------------
   result2.locate(5);
   SqlResult result3(connection);
   ASSERT_TRUE(result3.runQuery("SELECT * FROM sampledata ORDER BY id ASC"));
   ASSERT_EQ(result3.totalRows(), 10);
   ASSERT_EQ(result3.totalColumns(), 14);
   result3.moveLast();

   EXPECT_EQ( result3.getStringValue("id"),           "10");
   EXPECT_EQ( result3.getStringValue("val_varchar"),  result2.getStringValue("val_varchar") /* u8"שלום 2"_asChar             */);
   EXPECT_EQ( result3.getStringValue("val_text"),     result2.getStringValue("val_text")    /* u8"世界您好！ QQQQQQ2"_asChar  */);
   EXPECT_EQ( result3.getStringValue("val_bigint"),   "2222222222222222222");
   EXPECT_EQ( result3.getStringValue("val_char"),     u8"ППППП02             "_asChar );    // DB CHAR, Note the spaces
   EXPECT_EQ( result3.getStringValue("val_date"),     "2000-07-21");
   EXPECT_EQ( result3.getStringValue("val_time"),     "12:20:00");
   EXPECT_EQ( result3.getStringValue("val_datetime"), "2002-02-11 19:55:00");
   EXPECT_EQ( result3.getStringValue("val_real"),     "22.123457");              // DB REAL,   rounded by C Compiler
   EXPECT_EQ( result3.getStringValue("val_double"),   "24.123456789012344");     // DB DOUBLE, rounded by C Compiler
   EXPECT_EQ( result3.getStringValue("val_numeric"),  "234446666772.45");        // 
   EXPECT_EQ( result3.getStringValue("val_bool"),     "false");                  // libpq received as "f", tobasasql normalized it as "false"
   EXPECT_EQ( result3.getStringValue("val_binary1"),   "");
   EXPECT_EQ( util::toUpper(result3.getStringValue("val_binary")), "322E20544F42415341");
}


} // namespace test_pgsql