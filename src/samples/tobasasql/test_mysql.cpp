#if defined(TOBASA_SQL_USE_MYSQL)

#include <string>
#include <iostream>
#include <tobasa/datetime.h>
#include <tobasa/logger.h>
#include <tobasasql/sql_driver.h>
#include <tobasasql/sql_connection.h>
#include <tobasasql/sql_query.h>
#include <tobasasql/sql_table.h>

#include "util_u8.h"

/*
   Sample connection string:
   "Database=tbs_coba;User=tbs_user;Password=xxxxxxxxxx;Server=127.0.0.1;Port=3306"
*/

namespace testmysql {

   const std::string dropTableRawData("DROP TABLE IF EXISTS rawdata");

   const std::string createTableRawdata(
   R"-(
         CREATE TABLE IF NOT EXISTS rawdata (
            id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
            note VARCHAR(10) NULL,
            rawdata TEXT NULL,
            code INT NULL
         );
   )-");

   const std::string dropTableSampleDataTypes("DROP TABLE IF EXISTS sampledatatypes");
   const std::string createTableSampleDataTypes(
   R"-(
         CREATE TABLE IF NOT EXISTS sampledatatypes (
            id          INT               AUTO_INCREMENT PRIMARY KEY,
            vchar       char(20)              NULL, -- MYSQL_TYPE_STRING (254)
            vvarchar    VARCHAR(20)           NULL, -- MYSQL_TYPE_VAR_STRING (253)
            vbinary     BINARY(20)            NULL, -- MYSQL_TYPE_VAR_STRING (253)
            vvarbinary  VARBINARY(20)         NULL, -- MYSQL_TYPE_VAR_STRING (253)
            vtinyblob   TINYBLOB              NULL, -- MYSQL_TYPE_BLOB (252)
            vtinytext   TINYTEXT              NULL, -- MYSQL_TYPE_BLOB (252)
            vtext       TEXT(100)             NULL, -- MYSQL_TYPE_BLOB (252)
            vblob       BLOB(100)             NULL, -- MYSQL_TYPE_BLOB (252)
            vmediumtext MEDIUMTEXT            NULL, -- MYSQL_TYPE_BLOB (252)
            vmediumblob MEDIUMBLOB            NULL, -- MYSQL_TYPE_BLOB (252)
            vlongtext   LONGTEXT              NULL, -- MYSQL_TYPE_BLOB (252)
            vlongblob   LONGBLOB              NULL, -- MYSQL_TYPE_BLOB (252)
            vbit        BIT(16)               NULL, -- MYSQL_TYPE_BIT (16)
            vtinyint    TINYINT               NULL, -- MYSQL_TYPE_TINY (1)
            vbool       BOOL                  NULL, -- MYSQL_TYPE_TINY (1)
            vboolean    BOOLEAN               NULL, -- MYSQL_TYPE_TINY (1)
            vsmallint   SMALLINT              NULL, -- MYSQL_TYPE_SHORT (2)
            vmediumint  MEDIUMINT             NULL, -- MYSQL_TYPE_INT24 (9)
            vint        INT                   NULL, -- MYSQL_TYPE_LONG (3)
            vinteger    INTEGER               NULL, -- MYSQL_TYPE_LONG (3)
            vbigint     BIGINT                NULL, -- MYSQL_TYPE_LONGLONG (8)
            vubigint    BIGINT UNSIGNED       NULL, -- MYSQL_TYPE_LONGLONG (8) -- unsigned long long
            vfloat      FLOAT(9,3)            NULL, -- MYSQL_TYPE_FLOAT (4)
            vfloat24    FLOAT(24)             NULL, -- MYSQL_TYPE_FLOAT (4)
            vfloat53    FLOAT(53)             NULL, -- MYSQL_TYPE_DOUBLE (5)
            vdouble     DOUBLE(9,3)           NULL, -- MYSQL_TYPE_DOUBLE (5)
            vdoublep    DOUBLE PRECISION(9,3) NULL, -- MYSQL_TYPE_DOUBLE (5)
            vdecimal    DECIMAL(12,6)         NULL, -- MYSQL_TYPE_NEWDECIMAL (246) -- NUMERIC
            vdec        DEC                   NULL, -- MYSQL_TYPE_NEWDECIMAL (246)
            vdate       DATE                  NULL, -- MYSQL_TYPE_DATE (10)
            vdatetime   DATETIME              NULL, -- MYSQL_TYPE_DATETIME (12)
            vtimestamp  TIMESTAMP             NULL, -- MYSQL_TYPE_TIMESTAMP (7)
            vtime       TIME                  NULL, -- MYSQL_TYPE_TIME (11)
            vyear       YEAR                  NULL  -- MYSQL_TYPE_YEAR (13)
         );
   )-");

   const std::string insertTableSampleDataTypes(
   u8R"-(
         INSERT INTO sampledatatypes
            ( vbigint,               vvarchar,       vtext,               vchar,      vdate,        vtime,    vdatetime,          vfloat,       vdouble,              vdecimal,    vtinyint,  vbit,   vbinary)
         VALUES
            ( 111111111111111111,   'こんにちは 1', 'Hello World! QQQ1',  'ППППП01',  '1999-05-20',  '11:20', '2001-01-24 22:36',  11.123456789,  14.1234567890123456,  134446666771.45, 1, b'10101010', 0x312E20544F42415341),
            ( 2222222222222222222,  'שלום 2',      '世界您好！ QQQQQQ2', 'ППППП02',  '2000-07-21',  '12:20', '2002-02-11 19:55',  22.123456789,  24.1234567890123456,  234446666772.45,  0, b'10101011', 0x322E20544F42415341),
            ( 9223372036854775806,  'Ողջույն 3',   'Բարև աշխարհ! QQQ3',  'ППППП03',  '2001-04-18',  '03:40', '2003-03-14 15:21',  33.123456789,  34.1234567890123456,  334446666773.45,  1, b'10101110', 0x332E20544F42415341),
            ( 9223372036854775807,  'Привет 4',    'Γεια σου κόσμε! 4',  'ППППП04',  '2002-04-18',  '04:40', '2004-04-14 15:21',  44.123456789,  44.1234567890123456,  433444666774.45,  0, b'10101110', 0x342E20544F42415341);
   )-"_asChar );

} // testmysql


uint64_t MysqlBitToUInt64(const char* data, unsigned long length)
{
   uint64_t value = 0;

   for (unsigned long i = 0; i < length; ++i)
   {
      value <<= 8;
      value |= static_cast<unsigned char>(data[i]);
   }

   return value;
}

std::string ToMysqlBytesStr(uint64_t val)
{
   std::string value;

   if (val == 0)
      return std::string(1, '\0');

   while (val != 0)
   {
      value.insert(value.begin(), static_cast<char>(val & 0xFFu));
      val >>= 8;
   }

   return value;
}

std::vector<uint8_t> ToMysqlBytes(uint64_t value)
{
   std::vector<uint8_t> bytes;
   while (value != 0)
   {
      bytes.insert(bytes.begin(), static_cast<uint8_t>(value & 0xFFu));
      value >>= 8;
   }
   if (bytes.empty())
      bytes.push_back(0);

   return bytes;
}

std::vector<uint8_t> ToMysqlBitBytesFromLiteral(const std::string& s)
{
   std::string bits = s;

   // accept "b'00001100'" or "00001100"
   if (bits.size() >= 3 && bits.rfind("b'", 0) == 0 && bits.back() == '\'')
      bits = bits.substr(2, bits.size() - 3);

   if (bits.empty())
      return {0};

   std::vector<uint8_t> out((bits.size() + 7) / 8, 0);

   for (size_t i = 0; i < bits.size(); ++i)
   {
      if (bits[i] != '0' && bits[i] != '1')
         throw std::invalid_argument("invalid bit string");

      size_t byteIndex = i / 8;
      size_t bitIndex  = 7 - (i % 8);

      if (bits[i] == '1')
         out[byteIndex] |= (uint8_t(1) << bitIndex);
   }

   return out;
}

int main()
{
   using namespace tbs;
   using namespace testmysql;

   // We need this for Tobasa DateTime objects or SQL date/time conversion:
   if (! tbs::DateTime::initTimezoneData())
      return 1;

   std::cout << "TOBASA SQL Drivers\n";
   
   // We have to set log target for tbs::Logger
   tbs::Logger::setTarget(new tbs::log::CoutLogSink()) ;

   try
   {
      using namespace tbs::sql;
      using SqlParameterCollection = MysqlParameterCollection ;
      using SqlParameter = MysqlParameter;

      std::string connString = "Database=tbs_coba;User=tbs_user;Password=AdmBaru98;Server=10.62.22.2;Port=3306";
      SqlConnection<MysqlDriver> conn;
      if (!conn.connect(connString))
         return 1;

      conn.setLogSqlQuery(true);
      conn.setLogSqlQueryInternal(true);
      conn.setLogExecuteStatus(true);

      if (!conn.executeVoid(dropTableRawData))
         throw std::runtime_error("Could not drop rawdata table");

      if (!conn.executeVoid(dropTableSampleDataTypes))
         throw std::runtime_error("Could not drop sampledatatypes table");

      if (!conn.executeVoid(createTableRawdata))
         throw std::runtime_error("Could not create rawdata table");

      if (!conn.executeVoid(createTableSampleDataTypes))
         throw std::runtime_error("Could not create sampledatatypes table");

      if (!conn.executeVoid(insertTableSampleDataTypes))
         throw std::runtime_error("Could not insert into sampledatatypes table");

      bool runTest1  = false;
      bool runTest2  = false;
      bool runTest3  = false;
      bool runTest4  = true;
      bool runTest5  = false;
      bool runTest6  = false;
      bool runTest7  = false;
      bool runTest8  = false;
      bool runTest9  = false;

      // ---------------------------------------------------------
      if (runTest1)
      {
         auto currentTime = conn.executeScalar("SELECT NOW()");
         std::cout << "MYSQL Driver\n";
         std::cout << "Current time    : " << currentTime << "\n";
         std::cout << "Backend Version : " << conn.versionString() << "\n";

         auto numVal = conn.executeScalar("SELECT CAST(345 AS UNSIGNED)");
         std::cout << "numVal int      : " << numVal << "\n";

         auto bitVal1 = conn.executeScalar("SELECT b'1000001'"); // MariaDB client library returns the value with internal type MYSQL_TYPE_VAR_STRING
         std::cout << "bitVal 1        : " << bitVal1 << "\n";

         auto bitVal2 = conn.executeScalar("SELECT vbit FROM sampledatatypes"); // MariaDB client library treat this as MYSQL_TYPE_BIT
         std::cout << "bitVal 2        : " << bitVal2 << "\n";

         auto numVal2 = conn.executeScalar("SELECT CAST(34.54 AS DOUBLE)");
         std::cout << "numVal2 double  : " << numVal2 << "\n";
      }

      // ---------------------------------------------------------
      if (runTest2)
      {
         using VariantHelper = MysqlVariantHelper;

         SqlParameterCollection param;
         param.push_back( std::make_shared<SqlParameter>("id",  DataType::bigint, (int32_t)1) );
         auto res0 = conn.executeScalar("SELECT * FROM sampledatatypes WHERE id = ?", param);
         auto res1 = conn.executeScalar("SELECT * FROM sampledatatypes");
         auto res2 = conn.execute("SELECT vchar, vvarchar FROM sampledatatypes");

         SqlResult<MysqlDriver> sqlResult(conn);
         sqlResult.setOptionCacheData(true);
         sqlResult.setOptionOpenTable(false);
         sqlResult.runQuery("SELECT * FROM sampledatatypes LIMIT 1 OFFSET 0");

         if (sqlResult.isValid() && sqlResult.totalRows() > 0)
         {
            long totalRows       = sqlResult.totalRows();
            sqlResult.moveLast();

            auto vdate         = sqlResult.getVariantValue("vdate");
            auto vdatetime     = sqlResult.getVariantValue("vdatetime");
            auto vtimestamp    = sqlResult.getVariantValue("vtimestamp");
            auto vtime         = sqlResult.getVariantValue("vtime");
            auto vyear         = sqlResult.getVariantValue("vyear");
            auto vbit          = sqlResult.getVariantValue("vbit");
            auto vubigint      = sqlResult.getVariantValue("vubigint");

            auto vdateStr      = VariantHelper::toString(vdate);
            auto vdatetimeStr  = VariantHelper::toString(vdatetime);
            auto vtimestampStr = VariantHelper::toString(vtimestamp);
            auto vtimeStr      = VariantHelper::toString(vtime);
            auto vyearStr      = VariantHelper::toString(vyear);
            auto vbitStr       = VariantHelper::toString(vbit);
            auto vubigintStr   = VariantHelper::toString(vubigint);
            
            auto vdateS        = sqlResult.getStringValue("vdate");
            auto vdatetimeS    = sqlResult.getStringValue("vdatetime");
            auto vtimestampS   = sqlResult.getStringValue("vtimestamp");
            auto vtimeS        = sqlResult.getStringValue("vtime");
            auto vyearS        = sqlResult.getStringValue("vyear");
            auto vbitS         = sqlResult.getStringValue("vbit");
            auto vubigintS     = sqlResult.getStringValue("vubigint");
         }
      }

      // ---------------------------------------------------------
      if (runTest3)
      {
         // bool foo = "WEWEWEWEWE";  // In C++, the conversion from a string literal (like "WEWEWEWEWE") to a boolean value is allowed. 

         SqlParameterCollection param;
         param.push_back(std::make_shared<SqlParameter>("vchar",      DataType::character, std::string("WEWEWEWEWE")));
         param.push_back(std::make_shared<SqlParameter>("vvarchar",   DataType::varchar,   std::string("HHHHHHHHHHHHHHHHHHHH") ));

         // For MySQL BIT columns, in bound parameters, we should send the raw bit bytes
         //param.push_back(std::make_shared<SqlParameter>("vbit",       DataType::varbit,    ToMysqlBytes(12) ));
         param.push_back(std::make_shared<SqlParameter>("vbit",       DataType::varbit,    ToMysqlBitBytesFromLiteral("b'00011100'") ));

         param.push_back(std::make_shared<SqlParameter>("vbigint",    DataType::bigint,    9223372036854775807));

         param.push_back(std::make_shared<SqlParameter>("vubigint",   DataType::bigint,    18446744073709551615));

         // Use HEX encoded string as blob data source
         param.push_back(std::make_shared<SqlParameter>("vblob",      DataType::varbinary, std::string("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"), 24));
         // Use std::vector<uint8_t> as blob data source
         std::vector<uint8_t> blobdata(24);
         conv::hexDecode(std::string("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"), blobdata.data());
         param.push_back(std::make_shared<SqlParameter>("vmediumblob", DataType::varbinary, std::move(blobdata), 24));


         auto affectedRows = conn.execute("INSERT INTO sampledatatypes (vchar,vvarchar,vbit,vbigint,vubigint,vblob,vmediumblob) values (?, ? , ?, ?, ?, ?, ? )", param);
         if (affectedRows>0)
         {
            auto lastId = conn.lastInsertRowid();
            SqlParameterCollection par0;
            par0.push_back(std::make_shared<SqlParameter>("id",  DataType::bigint, lastId));
            auto res0 = conn.executeScalar("SELECT vmediumblob FROM sampledatatypes WHERE id = ?", par0);

            SqlParameterCollection par1;
            par1.push_back(std::make_shared<SqlParameter>("vbit",  DataType::bigint, lastId));
            auto res1 = conn.executeScalar("SELECT vbit FROM sampledatatypes WHERE id = ?", par1);
            auto x=1;
         }
      }



      // ---------------------------------------------------------
      if (runTest4)
      {
         std::string sql("INSERT INTO sampledatatypes (vbigint,vubigint) values (?, ?)");
         SqlQuery<MysqlDriver> query(conn, sql, ParameterStyle::native);
         // send the maximum uint64_t value to normal BIGINT column, throwing exception
         //query.addParam("vbigint",    DataType::bigint,    18446744073709551615);
         query.addParam("vbigint",    DataType::bigint,     -9223372036854775807);

         // send the maximum uint64_t value to an unsigned BIGINT column - OK
         query.addParam("vubigint",   DataType::bigint,    9223372036854775807/*18446744073709551615*/);

         auto affectedRows = query.execute();
         if (affectedRows>0)
         {
            int64_t lastId = conn.lastInsertRowid();
            if (lastId>0)
            {
               SqlParameterCollection param;
               param.push_back(std::make_shared<SqlParameter>("id",  DataType::bigint, lastId));
               auto res = conn.executeScalar("SELECT vubigint FROM sampledatatypes WHERE id = ?", param);
               auto x=1;
            }
         }
      }



      // ---------------------------------------------------------
      if (runTest5)
      {
         SqlTable<MysqlDriver> table(conn, "sampledatatypes");
         table.init();
         if (table.isValid())
         {
            table.moveFirst();
            auto a = table.getStringValue("vdecimal");
            auto b = table.getVariantValue("vbigint");
            auto c = table.getVariantValue("vbit");
            auto d = table.getStringValue("vdate");
            auto e = table.getStringValue("vdatetime");
            auto f = table.getStringValue("vtimestamp");
            auto g = table.getStringValue("vtime");
            auto h = table.getStringValue("vyear");
            auto x = 1;
         }
      }



      // ---------------------------------------------------------
      if (runTest6)
      {
         SqlParameterCollection param;
         param.push_back(std::make_shared<SqlParameter>("id",       DataType::integer, (long)3));
         param.push_back(std::make_shared<SqlParameter>("vbigint",  DataType::bigint, (int64_t)9223372036854775806));

         auto res0 = conn.execute("SELECT vdatetime FROM sampledatatypes WHERE id = ? AND vbigint = ?", param);
         auto res1 = conn.execute("UPDATE sampledatatypes SET val_date='2002-04-03' WHERE id = ? AND vbigint = ?", param);

         auto res2 = conn.executeScalar("SELECT vdatetime FROM sampledatatypes WHERE id = ? AND vbigint = ?", param);
         auto res3 = conn.executeScalar("UPDATE sampledatatypes SET val_date='2002-04-27' WHERE id = ? AND vbigint = ?", param);
         auto x=1;
      }



      // ---------------------------------------------------------
      if (runTest7)
      {
         auto res4 = conn.execute("UPDATE sampledatatypes SET vdate='2002-04-10' WHERE id = 3 AND vbigint = 9223372036854775806");
         auto res5 = conn.execute("SELECT vdatetime FROM sampledatatypes WHERE id = 3 AND vbigint = 9223372036854775806");
         auto res6 = conn.executeScalar("SELECT CAST(345 AS UNSIGNED)");
         auto res7 = conn.executeScalar("UPDATE sampledatatypes SET vdate='2002-04-09' WHERE id = 3 AND vbigint = 9223372036854775806");
         auto x=1;
      }



      // ---------------------------------------------------------
      // Variant type test
      if (runTest8)
      {
         using VariantType = MysqlVariantType;
         using VariantHelper = MysqlVariantHelper;

         SqlResult<MysqlDriver> sqlResult(conn);
         sqlResult.setOptionCacheData(true);
         sqlResult.setOptionOpenTable(false);

         sqlResult.runQuery("SELECT * FROM rawdata LIMIT 1 OFFSET 0");
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
         }
      }



      // ---------------------------------------------------------
      if (runTest9)
      {
         using SqlParameter = MysqlParameter;
         using VariantType = MysqlVariantType;

         auto paramSelectId = std::make_shared<SqlParameter>("id", sql::DataType::integer, 1/*, sizeof(int)*/);

         sql::SqlTable<MysqlDriver> table(conn, "sampledatatypes");
         table.getRetrieveDataOption()
            .pageSize(100)              // 10 records per page
            .pagePosition(1)           // set page position
            .orderBy("id ASC")
            .getParameters().push_back(paramSelectId);   // sql query now has one parameter => id

         table.init();

         if (table.isValid())
         {
            DateTime datetime;
            datetime.parse("2000-12-31 00:00:00");

            // TEST INSERT VariantType, make sure appendRows() first!
            //datetime.timePoint() += std::chrono::years{ 1 } + std::chrono::months{ 1 } + std::chrono::days{ 1 } + hours{ 1 } + minutes{ 1 } + seconds{1};
            datetime.timePoint() += tbsdate::years{ 5 };

            table.appendRows(1);
            table.moveLast();
            //table.setValue("vbigint",     VariantType(9000000000000000000 + 111));
            table.setValue("vvarchar",    VariantType(std::string("ꦲꦭꦺꦴ") + "111"));
            table.setValue("vtext",       VariantType(std::string("77 ꦏꦢꦺꦴꦱ꧀ꦥꦸꦤ꧀ꦢꦶꦏꦧꦂꦫꦶꦥꦸꦤ? ") + "111"));
            table.setValue("vdatetime",   VariantType(datetime.format("{:%Y-%m-%d %H:%M:%S}")));
            table.setValue("vdate",       VariantType(datetime.format("{:%Y-%m-%d}")));
            table.setValue("vtime",       VariantType(datetime.format("{:%H:%M:%S}")));
            //table.setValue("vchar",       VariantType(std::string("Welcome") + "111"));
            table.setValue("vfloat",       VariantType((float)27.1234567890 + (float)1)); // float
            table.setValue("vdouble",     VariantType((double)37.12345678901234567890 + (double)1)); // double or real
            table.setValue("vdecimal",    VariantType(std::string("373444666778.68"))); // numeric
            table.setValue("vbool",       VariantType(false));
            table.setValue("vbigint",     std::monostate{} );
            table.setValue("vchar",       std::monostate{} );
            table.saveTable();
         }
      }


      std::cout << "\n";
      conn.disconnect();
   }
   catch(const std::exception& ex)
   {
      std::cerr << "Exception : " << ex.what();
   }

   return 0;
}

#endif