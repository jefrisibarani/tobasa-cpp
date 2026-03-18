#pragma once

#include <string>
#include "util_u8.h"

/*
   Sample connection string:
   "Driver={MySQL ODBC 8.0 Unicode Driver};SERVER=10.62.22.2;PORT=3306;DATABASE=hmswebsvc;USER=hmswebsvc;PASSWORD=xxxxx;charset=utf8"

   Note: we must set option charset=utf8, to get unicode data from server correctly.
*/

namespace testodbc_mysql {

   using namespace tbs::test;

   const std::string dropTableSampleData("DROP TABLE IF EXISTS sampledata");

   const std::string createTableSampleData(
   R"-(
         CREATE TABLE IF NOT EXISTS sampledata (
            id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
            val_varchar       VARCHAR(20)     NULL,
            val_text          TEXT            NULL,
            val_bigint        BIGINT          NULL,
            val_char          CHAR(20)        NULL,
            val_date          DATE            NULL,
            val_time          TIME            NULL,
            val_datetime      DATETIME        NULL,
            val_real          FLOAT           NULL,
            val_double        DOUBLE          NULL,
            val_numeric       NUMERIC(14,2)   NULL,
            val_bool          TINYINT(1)      NULL,
            val_binary        BINARY(100)     NULL,
            val_binary1       VARBINARY(4096) NULL,
            val_datetime1     DATETIME        NULL,
            val_smalldatetime DATETIME        NULL
         ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
   )-");



   /*
      Note:
      On Linux with unixODBC, unicode characters inside this query sent successfully out and get corrupted.
      On Windows no problem found.
      So as a work around, on Linux we send all the fields inside a struct(see getSampleDataVector())
      then send it using prepared statement with bound variables.
      Instead of :
         sqlConn.executeVoid(testodbc_mysql::insertTableSampleData);

      We do this:

         auto vSampleData = testodbc_mysql::getSampleDataVector();
         for (auto &row : vSampleData) // access by reference to avoid copying
         {
            sql::SqlQuery<SqlDriverType> query(_sqlConn);
            std::string sql =
               tbsfmt::format(R"-(
                  INSERT INTO sampledata (val_bigint, val_varchar, val_text, val_char, val_bool, val_real,
                                       val_double, val_numeric, val_date, val_time, val_datetime, val_binary)
                  VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) )-" );

            query.command(sql)
                  .addParam("bigint",   sql::DataType::bigint,     row.valBigint)
                  .addParam("varchar",  sql::DataType::varchar,    row.valVarchar)
                  .addParam("text",     sql::DataType::text,       row.valText)
                  .addParam("char",     sql::DataType::character,  row.valChar)
                  .addParam("bool",     sql::DataType::boolean,    row.valBool)
                  .addParam("float",    sql::DataType::float4,     row.valReal)
                  .addParam("double",   sql::DataType::float8,     row.valDouble)
                  .addParam("numeric",  sql::DataType::numeric,    row.valNumeric)
                  .addParam("date",     sql::DataType::date,       row.valDate)
                  .addParam("time",     sql::DataType::time,       row.valTime)
                  .addParam("datetime", sql::DataType::timestamp,  row.valDatetime)
                  .addParam("binary",   sql::DataType::varbinary,  row.valBinary,  static_cast<long>(row.valBinary.length()/2) );
            query.execute();
         }
   */

   const std::string insertTableSampleData(
   u8R"-(
         INSERT INTO sampledata
            ( val_bigint,          val_varchar,     val_text,             val_char,     val_date,     val_time,   val_datetime,       val_real,      val_double,          val_numeric, val_bool, val_binary)
         VALUES
            ( 111111111111111111,   'こんにちは 1', 'Hello World! QQQ1',  'ППППП01',  '1999-05-20',  '11:20', '2001-01-24 22:36',  11.123456789,  14.1234567890123456,  134446666771.45,  1, 0x312E20544F42415341),
            ( 2222222222222222222,  'שלום 2',      '世界您好！ QQQQQQ2', 'ППППП02',  '2000-07-21',  '12:20', '2002-02-11 19:55',  22.123456789,  24.1234567890123456,  234446666772.45,  0, 0x322E20544F42415341),
            ( 9223372036854775806,  'Ողջույն 3',   'Բարև աշխարհ! QQQ3',  'ППППП03',  '2001-04-18',  '03:40', '2003-03-14 15:21',  33.123456789,  34.1234567890123456,  334446666773.45,  1, 0x332E20544F42415341),
            ( 9223372036854775807,  'Привет 4',    'Γεια σου κόσμε! 4',  'ППППП04',  '2002-04-18',  '04:40', '2004-04-14 15:21',  44.123456789,  44.1234567890123456,  433444666774.45,  0, 0x342E20544F42415341);
   )-"_asChar );


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

} // namespace testodbc_mysql
