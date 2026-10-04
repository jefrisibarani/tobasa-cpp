#include <string>
#include "tobasa_test_support/util_u8.h"

namespace test_sqlite {

using namespace tbs::test_support;

const std::string dropTableSampleData("DROP TABLE IF EXISTS sampledata");

const std::string createTableSampleData(
R"-(
      CREATE TABLE sampledata (
         id             integer not null, -- 0
         val_varchar    varchar(20),      -- 1
         val_text       text,             -- 2
         val_bigint     bigint,           -- 3
         val_char       char(20),         -- 4
         val_date       date,             -- 5
         val_time       time,             -- 6
         val_datetime   datetime,         -- 7
         val_real       real,             -- 8
         val_double     double,           -- 9
         val_numeric    numeric,          -- 10
         val_bool       boolean,          -- 11
         val_binary     blob,             -- 12
         val_binary1    blob,             -- 13
         PRIMARY KEY("id" AUTOINCREMENT)
      );
)-");

// Note: 
//  +--------------+------------------------+------------------------+------------------------+-------------------------+
//  | Column Name  | sqlite3_column_type()  | Value send as sql text | sqlite3_column_text    | sqlite3_column_double() |
//  +--------------+------------------------+------------------------+------------------------+-------------------------+
//  | val_double   | SQLITE_FLOAT           | 44.1234567890123456    | 44.1234567890123       | 44.123456789012344      |
//  | val_real     | SQLITE_FLOAT           | 44.123456789           | 44.123456789           | 44.123456789000002      |
//  | val_numeric  | SQLITE_FLOAT           | 433444666774.45        | 433444666774.45        | 433444666774.45001      |
//  | val_time     | SQLITE_TEXT            | 04:40                  | 04:40                  |                         |
//  | val_datetime | SQLITE_TEXT            | 2004-04-14 15:21       | 2004-04-14 15:21       |                         |
//  +--------------+------------------------+------------------------+------------------------+-------------------------+
const std::string insertTableSampleData(
u8R"-(
      INSERT INTO sampledata
         (  val_bigint,          val_varchar,   val_text,            val_char,     val_date,     val_time,    val_datetime,       val_real,        val_double,        val_numeric,    val_bool, val_binary)
      VALUES
         ( 111111111111111111,   'こんにちは 1', 'Hello World! QQQ1',  'ППППП01',  '1999-05-20',  '11:20',   '2001-01-24 22:36',  11.123456789,  14.1234567890123456,  134446666771.45,  1, x'312E20544F42415341'),
         ( 2222222222222222222,  'שלום 2',     '世界您好！ QQQQQQ2',   'ППППП02',  '2000-07-21',  '12:20',   '2002-02-11 19:55',  22.123456789,  24.1234567890123456,  234446666772.45,  0, x'322E20544F42415341'),
         ( 9223372036854775806,  'Ողջույն 3',   'Բարև աշխարհ! QQQ3', 'ППППП03',  '2001-04-18',  '03:40',   '2003-03-14 15:21',  33.123456789,  34.1234567890123456,  334446666773.45,  1, x'332E20544F42415341'),
         ( 9223372036854775807,  'Привет 4',   'Γεια σου κόσμε! 4',  'ППППП04',  '2002-04-18',  '04:40',   '2004-04-14 15:21',  44.123456789,  44.1234567890123456,  433444666774.45,  0, x'342E20544F42415341');
)-"_asChar );


const std::string dropTableRawData("DROP TABLE IF EXISTS rawdata");

const std::string createTableRawdata(
R"-(
      CREATE TABLE rawdata (
         id integer not null,
         note VARCHAR(10),
         rawdata text,
         code integer,
         PRIMARY KEY("id" AUTOINCREMENT)
      )
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

// Note: 
//  +--------------+------------------------+------------------------+------------------------+-------------------------+
//  | Column Name  | sqlite3_column_type()  | Value send as param    | sqlite3_column_text    | sqlite3_column_double() |
//  +--------------+------------------------+------------------------+------------------------+-------------------------+
//  | val_double   | SQLITE_FLOAT           | 44.1234567890123456l   | 44.1234567890123       | 44.123456789012344      |
//  | val_real     | SQLITE_FLOAT           | 44.123456789f          | 44.123456789           | 44.123456789000002      |
//  | val_numeric  | SQLITE_FLOAT           | "433444666774.45"      | 433444666774.45        | 433444666774.45001      |
//  | val_time     | SQLITE_TEXT            | 04:40                  | 04:40                  |                         |
//  | val_datetime | SQLITE_TEXT            | 2004-04-14 15:21       | 2004-04-14 15:21       |                         |
//  +--------------+------------------------+------------------------+------------------------+-------------------------+
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


} // namespace test_sqlite