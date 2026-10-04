#include <string>
#include "tobasa_test_support/util_u8.h"
/*
   NOTE: Save this file as UTF-8 with BOM so the Unicode text in this test is preserved correctly.
   Sample connection string:

*/

using namespace tbs::test_support;

namespace test_odbc {

const std::string dropTableSampleData(
R"-(  IF OBJECT_ID('dbo.sampledata', 'U') IS NOT NULL DROP TABLE dbo.sampledata;      )-");

const std::string createTableSampleData(
R"-(
      CREATE TABLE sampledata (
         id                int            IDENTITY(1,1) PRIMARY KEY, -- 0
         val_varchar       nvarchar(20)   NULL,                      -- 1
         val_text          ntext          NULL,                      -- 2
         val_bigint        bigint         NULL,                      -- 3
         val_char          nchar(20)      NULL,                      -- 4
         val_date          date           NULL,                      -- 5  since SQL Server 2008 (10.0) - and datetimeoffset
         val_time          time(7)        NULL,                      -- 6  since SQL Server 2008 (10.0)
         val_datetime      datetime2(7)   NULL,                      -- 7  since SQL Server 2008 (10.0)
         val_real          real           NULL,                      -- 8  c++ float
         val_double        float          NULL,                      -- 9  implied float(53) -> c++ double
         val_numeric       numeric(14,2)  NULL,                      -- 10
         val_bool          bit            NULL,                      -- 11
         val_binary        binary(9)      NULL,                      -- 12
         val_binary1       varbinary(max) NULL,                      -- 13
         val_datetime1     datetime       NULL,                      -- 14
         val_smalldatetime smalldatetime  NULL                       -- 15
      );
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
      for (auto &row : vSampleData)
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
         ( 111111111111111111,   N'こんにちは 1', N'Hello World! QQQ1', N'ППППП01',  '1999-05-20',  '11:20', '2001-01-24 22:36',  11.123456789,  14.1234567890123456,  134446666771.45,  1, 0x312E20544F42415341),
         ( 2222222222222222222,  N'שלום 2',     N'世界您好！ QQQQQQ2',  N'ППППП02',  '2000-07-21',  '12:20', '2002-02-11 19:55',  22.123456789,  24.1234567890123456,  234446666772.45,  0, 0x322E20544F42415341),
         ( 9223372036854775806,  N'Ողջույն 3',   N'Բարև աշխարհ! QQQ3', N'ППППП03',  '2001-04-18',  '03:40', '2003-03-14 15:21',  33.123456789,  34.1234567890123456,  334446666773.45,  1, 0x332E20544F42415341),
         ( 9223372036854775807,  N'Привет 4',   N'Γεια σου κόσμε! 4',  N'ППППП04',  '2002-04-18',  '04:40', '2004-04-14 15:21',  44.123456789,  44.1234567890123456,  433444666774.45,  0, 0x342E20544F42415341);
)-"_asChar );



const std::string dropTableRawData(
R"-(
      IF OBJECT_ID('dbo.rawdata', 'U') IS NOT NULL DROP TABLE dbo.rawdata;
)-");

const std::string createTableRawdata(
R"-(
      CREATE TABLE rawdata (
         id int IDENTITY(1,1) PRIMARY KEY,
         note varchar(10) NULL,
         rawdata text NULL,
         code int NULL
      )
)-");

const std::string populateTableRawdata(
R"-(
      DECLARE @max int
      DECLARE @it int

      DECLARE @_code int
      DECLARE @_note varchar(10)
      DECLARE @_rawdata varchar(MAX)

      SET @max = 20000
      SET @it = 0

      WHILE @it <= @max
      BEGIN
         SET @it = @it + 1
         SET @_code = @it
         SET @_note = 'C_' + CAST(@it as varchar)
         SET @_rawdata = 'Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam vel fringilla neque.. Aliquam quis feugiat metus. '
         INSERT INTO rawdata(note,rawdata,code) VALUES(@_note, @_rawdata, @_code)
      END
)-");


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
      {u8"こんにちは 1"_asChar, u8"Hello World! QQQ1"_asChar,  111111111111111111LL,  u8"ППППП01"_asChar, "1999-05-20",  "11:20",  "2001-01-24 22:36",  11.123456789f,  14.1234567890123456l,  "134446666771.45",  true,   "312E20544F42415341"},
      {u8"שלום 2"_asChar,      u8"世界您好！ QQQQQQ2"_asChar,  2222222222222222222LL, u8"ППППП02"_asChar, "2000-07-21", "12:20",   "2002-02-11 19:55",  22.123456789f,  24.1234567890123456l,  "234446666772.45",  false,  "322E20544F42415341"},
      {u8"Ողջույն 3"_asChar,   u8"Բարև աշխարհ! QQQ3"_asChar,  9223372036854775806LL, u8"ППППП03"_asChar, "2001-04-18",  "03:40",  "2003-03-14 15:21",  33.123456789f,  34.1234567890123456l,  "334446666773.45",  true,   "332E20544F42415341"},
      {u8"Привет 4"_asChar,    u8"Γεια σου κόσμε! 4"_asChar,  9223372036854775807LL, u8"ППППП04"_asChar,  "2002-04-18", "04:40",  "2004-04-14 15:21",  44.123456789f,  44.1234567890123456l,  "433444666774.45",  false,  "342E20544F42415341"}
   };

   return std::move(vector);
}

} // namespace test_odbc