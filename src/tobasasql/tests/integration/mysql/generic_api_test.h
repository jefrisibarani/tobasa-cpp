#include <string>
#include "tobasa_test_support/util_u8.h"

/*
   NOTE: Save this file as UTF-8 with BOM so the Unicode text in this test is preserved correctly.
   Sample connection string:

*/

using namespace tbs::test_support;

namespace test_mysql {

const std::string dropTableSampleData("DROP TABLE IF EXISTS sampledata");

const std::string createTableSampleData(
R"-(
      CREATE TABLE IF NOT EXISTS sampledata (
         id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,  -- 0
         val_varchar       VARCHAR(20)     NULL,      -- 1
         val_text          TEXT            NULL,      -- 2
         val_bigint        BIGINT          NULL,      -- 3
         val_char          CHAR(20)        NULL,      -- 4
         val_date          DATE            NULL,      -- 5
         val_time          TIME            NULL,      -- 6
         val_datetime      DATETIME        NULL,      -- 7
         val_real          FLOAT           NULL,      -- 8
         val_double        DOUBLE          NULL,      -- 9
         val_numeric       NUMERIC(14,2)   NULL,      -- 10
         val_bool          TINYINT(1)      NULL,      -- 11
         val_binary        BINARY(9)       NULL,      -- 12
         val_binary1       VARBINARY(4096) NULL,      -- 13
         val_datetime1     DATETIME        NULL,      -- 14
         val_smalldatetime DATETIME        NULL       -- 15
      ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
)-");


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

} // namespace test_mysql