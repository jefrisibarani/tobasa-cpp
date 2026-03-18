#pragma once

#include <string>
#include "util_u8.h"

/*
   Sample connection string:
   "Database=/home/dev/lisdb.db3;OpenCreate=True;OpenMemory=False;Password=xxxxx;"
*/

namespace testsqlite {

   using namespace tbs::test;

   const std::string dropTableSampleData("DROP TABLE IF EXISTS sampledata");

   const std::string createTableSampleData(
   R"-(
         CREATE TABLE sampledata (
            id             integer not null,
            val_varchar    varchar(20),
            val_text       text,
            val_bigint     bigint,
            val_char       char(20),
            val_date       date,
            val_time       time,
            val_datetime   datetime,
            val_real       real,
            val_double     double,
            val_numeric    numeric,
            val_bool       boolean,
            val_binary     blob,
            val_binary1    blob,
            PRIMARY KEY("id" AUTOINCREMENT)
         );
   )-");

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

} // namespace testsqlite
