#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "tobasa_test_support/environment.h"
#include "tobasa_test_support/util.h"
#include "tobasa_test_support/util_u8.h"

#include <tobasa/datetime.h>
#include "tobasasql/sql_driver.h"
#include "tobasasql/sql_connection.h"
#include "tobasasql/sql_result.h"
#include "tobasasql/sql_query.h"
#include "tobasasql/com_initializer.h"
#include "tobasasql/adodb_util.h"
#include "tobasasql/com_variant.h"

#include "generic_api_test.h"

/*
   NOTE: Save this file as UTF-8 with BOM so the Unicode text in this test is preserved correctly.
*/

namespace test_adodb {

using namespace tbs;
using namespace tbs::sql;
using namespace tbs::test_support;

using SqlConnection = sql::SqlConnection<sql::AdodbDriver>;
using SqlResult     = sql::SqlResult<sql::AdodbDriver>;
using SqlQuery      = sql::SqlQuery<sql::AdodbDriver>;
using SqlCommand    = sql::SqlQuery<sql::AdodbDriver>;


std::string getTime(const ComVariantType& variant)
{
   if (std::holds_alternative<std::vector<uint8_t>>(variant))
   {
      // When Driver={ODBC Driver 17 for SQL Server}
      // for SQL time, the data internally stored as std::vector<uint8_t>.
      // so we cannot get the real time value with getStringValue()
      return sql::adoDbTime2VariantToString(variant);
   }
   else 
   {
      auto originalTime = ComVariantHelper::toString(variant);
      return test_support::normalizeTime(originalTime);
   }
}


TEST(AdodbGenericAPIIntegrationTest, ConnectExecuteAndExecuteScalar)
{
   const std::string connectionString = loadEnvironmentValue(
      "TOBASA_ADODB_TEST_CONNECTION_SQLNCLI",
      std::string(TOBASA_PROJECT_SOURCE_DIR) + "/.env");

   if (connectionString.empty())
      GTEST_SKIP() << "Set TOBASA_ADODB_TEST_CONNECTION_SQLNCLI or add it to .env";

   ComInitializer comInitializer(true);
   
   SqlConnection conn;
   ASSERT_TRUE(  conn.connect(connectionString)        );
   EXPECT_EQ(    conn.execute("SELECT 1"), 0           );
   EXPECT_EQ(    conn.executeScalar("SELECT 1"), "1"   );

   SqlQuery      cmd(conn);
   
   ASSERT_TRUE(  cmd.query("SELECT 1", {}) );
   EXPECT_EQ(    cmd.execute(), 0          );
   EXPECT_EQ(    cmd.executeScalar(), "1"  );

   ASSERT_TRUE(  cmd.prepare("SELECT 1")   );

   EXPECT_EQ(    cmd.execute(), 0);
   EXPECT_EQ(    cmd.executeScalar(), "1"  );

   SqlResult     res(conn);
   ASSERT_TRUE(  res.runQuery("SELECT 1")    );
   EXPECT_EQ(    res.getStringValue(0), "1"  );
}

TEST(AdodbGenericAPIIntegrationTest, CRUDAndDataRetrieval)
{
   // We need this for Tobasa DateTime objects or SQL date/time conversion:
   ASSERT_TRUE(tbs::DateTime::initTimezoneData());

   const std::string connectionString = test_support::loadEnvironmentValue(
      "TOBASA_ADODB_TEST_CONNECTION_SQLNCLI", std::string(TOBASA_PROJECT_SOURCE_DIR) + "/.env");

   if (connectionString.empty())
      GTEST_SKIP() << "Set TOBASA_ADODB_TEST_CONNECTION_SQLNCLI or add it to .env";

   ComInitializer comInitializer(true);

   SqlConnection connection;

   // ------------------------------------------------------------------------------
   // Connect, initialize the database table, and verify its structure.
   // NOTE: we are using SQL Text directly
   // ------------------------------------------------------------------------------
   ASSERT_TRUE(connection.connect(connectionString));
   EXPECT_EQ(  connection.execute(dropTableSampleData),    0);
   EXPECT_EQ(  connection.execute(createTableSampleData),  0);
   EXPECT_EQ(  connection.execute(insertTableSampleData),  4);

   // Retrieve the result via SqlResult
   SqlResult result(connection);
   result.runQuery("SELECT * FROM sampledata ORDER BY id ASC");
   ASSERT_EQ( result.totalRows(),       4);
   ASSERT_EQ( result.totalColumns(),    16);
   
   // Retrive the result via SqlQuery
   SqlQuery command(connection);
   command.query("SELECT * FROM sampledata ORDER BY id ASC");
   auto pRes = command.executeResult();
   ASSERT_NE(pRes, nullptr);
   ASSERT_EQ(pRes->totalRows(), 4);
   ASSERT_EQ(pRes->totalColumns(), 16);

   // ------------------------------------------------------------------------------
   // Move to the last row and validate the field values.
   // ------------------------------------------------------------------------------
   pRes->moveLast();

   EXPECT_EQ( pRes->getStringValue("id"),           "4");
   EXPECT_EQ( pRes->getStringValue("val_varchar"),  u8"Привет 4"_asChar   );
   EXPECT_EQ( pRes->getStringValue("val_text"),     u8"Γεια σου κόσμε! 4"_asChar  );
   EXPECT_EQ( pRes->getStringValue("val_bigint"),   "9223372036854775807");
   EXPECT_EQ( pRes->getStringValue("val_char"),     u8"ППППП04             "_asChar );
   EXPECT_EQ( pRes->getStringValue("val_date"),          "2002-04-18"); 
   EXPECT_EQ( getTime(pRes->getVariantValue("val_time")), "04:40:00");
   EXPECT_EQ( normalizeDateTime(pRes->getStringValue("val_datetime")),      "2004-04-14 15:21:00");
   EXPECT_EQ( pRes->getStringValue("val_real"),     "44.123455");
   EXPECT_EQ( pRes->getStringValue("val_double"),   "44.123456789012344");
   EXPECT_EQ( pRes->getStringValue("val_numeric"),  "433444666774.45");
   EXPECT_EQ( pRes->getStringValue("val_bool"),     "false");
   EXPECT_EQ( pRes->getStringValue("val_binary1"),   "");
   EXPECT_EQ( util::toUpper(pRes->getStringValue("val_binary")), "342E20544F42415341");

   // ------------------------------------------------------------------------------
   // Read the third row values using typed accessors where appropriate.
   // ------------------------------------------------------------------------------
   pRes->locate(2);

   EXPECT_EQ( pRes->getLongValue(0),             3L);
   EXPECT_EQ( pRes->getStringValue(1),           u8ToString(u8"Ողջույն 3"));
   EXPECT_EQ( pRes->getStringValue(2),           u8ToString(u8"Բարև աշխարհ! QQQ3"));
   EXPECT_EQ( pRes->getLongLongValue(3),         9223372036854775806LL);
   EXPECT_EQ( pRes->getStringValue(4),           u8"ППППП03             "_asChar);
   EXPECT_EQ( pRes->getStringValue(5),           "2001-04-18");
   EXPECT_EQ( getTime(pRes->getVariantValue(6)), "03:40:00");
   EXPECT_EQ( normalizeDateTime(pRes->getStringValue(7)),           "2003-03-14 15:21:00");
   EXPECT_FLOAT_EQ(  pRes->getFloatValue(8),     33.123455f);
   EXPECT_DOUBLE_EQ( pRes->getDoubleValue(9),    34.123456789012344);
   EXPECT_EQ(        pRes->getStringValue(10),   "334446666773.45");
   EXPECT_TRUE(      pRes->getBoolValue(11));
   EXPECT_EQ(        pRes->getStringValue(13),  "");
   EXPECT_EQ( util::toUpper(pRes->getStringValue(12)), "332E20544F42415341");

   // ------------------------------------------------------------------------------
   // Back to the first row and decode the binary value from column 12.
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
   // INSERT data from getSampleDataVector()
   // ------------------------------------------------------------------------------
   std::string sql =
      tbsfmt::format(R"-(
         INSERT INTO sampledata (val_bigint, val_varchar, val_text, val_char, val_bool, val_real,
                                 val_double, val_numeric, val_date, val_time, val_datetime, val_binary)
         VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
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
      cmdInsert.addParam("float",    sql::DataType::float4,     row.valReal); 
      cmdInsert.addParam("double",   sql::DataType::float8,     row.valDouble);
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
   result2.runQuery("SELECT * FROM sampledata ORDER BY id ASC");
   ASSERT_EQ(result2.totalRows(), 8);
   ASSERT_EQ(result2.totalColumns(), 16);

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
      cmdInsert.addParam("float",    sql::DataType::float4,     result2.getFloatValue("val_real"));
      cmdInsert.addParam("double",   sql::DataType::float8,     result2.getDoubleValue("val_double"));
      // use std::string for sql numeric type, no need to include size,direction and decimal digit
      cmdInsert.addParam("numeric",  sql::DataType::numeric,    result2.getStringValue("val_numeric"));
      // use std::string for sql date,  format is yyyy-mm-dd
      cmdInsert.addParam("date",     sql::DataType::date,       result2.getStringValue("val_date") );
      // use std::string for sql time,  format is hh:mm:ss
      cmdInsert.addParam("time",     sql::DataType::time,       getTime(result2.getVariantValue("val_time")) );
      // use std::string for sql timestamp,  format is yyyy-mm-dd hh:mm:ss
      cmdInsert.addParam("datetime", sql::DataType::timestamp,  normalizeDateTime(result2.getStringValue("val_datetime")) );
      // use Hexadecimal encoded binary string for sql varbinary, and specify raw data's size
      auto hexData = result2.getStringValue("val_binary");
      cmdInsert.addParam("binary",   sql::DataType::varbinary,  hexData,  static_cast<long>(hexData.length()/2) );

      EXPECT_EQ( cmdInsert.execute(), 1);

      // reset the command for the next execute()
      cmdInsert.reset();

      result2.moveNext();
   }

   // ------------------------------------------------------------------------------
   // Retrieve the table, move to the last row and validate the field values with result2 6th rows(idx 5).
   // ------------------------------------------------------------------------------
   result2.locate(5);
   SqlResult result3(connection);
   result3.runQuery("SELECT * FROM sampledata ORDER BY id ASC");
   ASSERT_EQ(result3.totalRows(), 10);
   ASSERT_EQ(result3.totalColumns(), 16);
   result3.moveLast();

   EXPECT_EQ( result3.getStringValue("id"),           "10");
   EXPECT_EQ( result3.getStringValue("val_varchar"),  result2.getStringValue("val_varchar") /* u8"שלום 2"_asChar             */);
   EXPECT_EQ( result3.getStringValue("val_text"),     result2.getStringValue("val_text")    /* u8"世界您好！ QQQQQQ2"_asChar  */);
   EXPECT_EQ( result3.getStringValue("val_bigint"),   "2222222222222222222");
   EXPECT_EQ( result3.getStringValue("val_char"),     u8"ППППП02             "_asChar );

   EXPECT_EQ( result3.getStringValue("val_date"),           "2000-07-21");
   EXPECT_EQ( getTime(result3.getVariantValue("val_time")), "12:20:00");
   EXPECT_EQ( normalizeDateTime(result3.getStringValue("val_datetime")),       "2002-02-11 19:55:00");

   EXPECT_EQ( result3.getStringValue("val_real"),     "22.123457");
   EXPECT_EQ( result3.getStringValue("val_double"),   "24.123456789012344");
   EXPECT_EQ( result3.getStringValue("val_numeric"),  "234446666772.45");
   EXPECT_EQ( result3.getStringValue("val_bool"),     "false");
   EXPECT_EQ( result3.getStringValue("val_binary1"),   "");
   EXPECT_EQ( util::toUpper(result3.getStringValue("val_binary")), "322E20544F42415341");
}

} // namespace test_adodb