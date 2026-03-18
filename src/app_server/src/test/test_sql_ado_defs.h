#pragma once

#include <string>
#include "util_u8.h"

// https://docs.microsoft.com/en-us/sql/t-sql/data-types/ntext-text-and-image-transact-sql?view=sql-server-ver15
// Note: ntext, text, and image data types will be removed in a future version of SQL Server
//       Use nvarchar(max), varchar(max), and varbinary(max) instead

/*
   Sample connection string:
   "Provider=SQLNCLI11;Server=127.0.0.1;Database=coba;Uid=sa;Pwd=xxxxx;DataTypeCompatibility=80;"
*/


namespace testado {

using namespace tbs::test;

const std::string dropTableSampleData(
   R"-(
         IF OBJECT_ID('dbo.sampledata', 'U') IS NOT NULL
            DROP TABLE dbo.sampledata;
   )-");

const std::string dropTableSampleDataU(
   R"-(
         IF OBJECT_ID('dbo.sampledata_u', 'U') IS NOT NULL
            DROP TABLE dbo.sampledata_u;
   )-");

const std::string createTableSampleData(
   R"-(
         CREATE TABLE sampledata
         (
            id                int            IDENTITY(1,1) PRIMARY KEY,
            val_varchar       varchar(20)    NULL,
            val_text          text           NULL,
            val_bigint        bigint         NULL,
            val_char          char(20)       NULL,
            val_date          date           NULL,
            val_time          time(7)        NULL,
            val_datetime      datetime2(7)   NULL,
            val_real          real           NULL,
            val_double        float          NULL,
            val_numeric       numeric(14,2)  NULL,
            val_bool          bit            NULL,
            val_binary        binary(100)    NULL,
            val_binary1       varbinary(max) NULL,
            val_datetime1     datetime       NULL,
            val_smalldatetime smalldatetime  NULL
         );
   )-");

const std::string insertTableSampleData(
   R"-(
         INSERT INTO sampledata
            ( val_bigint,          val_varchar,     val_text,       val_char,     val_date,    val_time,   val_datetime,     val_real,     val_double,     val_numeric, val_bool, val_binary)
         VALUES
            ( 111111111111111111,   'HELLO 1',  'Hello World! 1',  'QWERTY01',  '1999-05-20',  '11:20',  '2001-01-24 22:36', 11.123456789,  14.1234567890123456,  134446666771.45,  1, 0x312E20544F42415341),
            ( 2222222222222222222,  'HELLO 2',  'Hello World! 2',  'QWERTY02',  '2000-07-21',  '12:20',  '2002-02-11 19:55', 22.123456789,  24.1234567890123456,  234446666772.45,  0, 0x322E20544F42415341),
            ( 9223372036854775806,  'HELLO 3',  'Hello World! 3',  'QWERTY03',  '2001-04-18',  '03:40',  '2003-03-14 15:21', 33.123456789,  34.1234567890123456,  334446666773.45,  1, 0x332E20544F42415341),
            ( 9223372036854775807,  'HELLO 4',  'Hello World! 4',  'QWERTY04',  '2002-04-18',  '04:40',  '2004-04-14 15:21', 44.123456789,  44.1234567890123456,  433444666774.45,  0, 0x342E20544F42415341);
   )-");


const std::string createTableSampleDataU(
   R"-(
         CREATE TABLE sampledata_u
         (
            id                int            IDENTITY(1,1) PRIMARY KEY,
            val_varchar       nvarchar(20)   NULL,
            val_text          ntext          NULL,
            val_bigint        bigint         NULL,
            val_char          nchar(20)      NULL,
            val_date          date           NULL,
            val_time          time(7)        NULL,
            val_datetime      datetime2(7)   NULL,
            val_real          real           NULL,
            val_double        float          NULL,
            val_numeric       numeric(14,2)  NULL,
            val_bool          bit            NULL,
            val_binary        binary(100)    NULL,
            val_binary1       varbinary(max) NULL,
            val_datetime1     datetime       NULL,
            val_smalldatetime smalldatetime  NULL
         );
   )-");

const std::string insertTableSampleDataU(
   u8R"-(
         INSERT INTO sampledata_u
            ( val_bigint,          val_varchar,     val_text,             val_char,     val_date,     val_time,   val_datetime,       val_real,      val_double,          val_numeric, val_bool, val_binary)
         VALUES
            ( 111111111111111111,   N'こんにちは 1', N'Hello World! QQQ1',  N'ППППП01',  '1999-05-20',  '11:20', '2001-01-24 22:36',  11.123456789,  14.1234567890123456,  134446666771.45,  1, 0x312E20544F42415341),
            ( 2222222222222222222,  N'שלום 2',     N'世界您好！ QQQQQQ2',   N'ППППП02',  '2000-07-21',  '12:20', '2002-02-11 19:55',  22.123456789,  24.1234567890123456,  234446666772.45,  0, 0x322E20544F42415341),
            ( 9223372036854775806,  N'Ողջույն 3',   N'Բարև աշխարհ! QQQ3', N'ППППП03',  '2001-04-18',  '03:40', '2003-03-14 15:21',  33.123456789,  34.1234567890123456,  334446666773.45,  1, 0x332E20544F42415341),
            ( 9223372036854775807,  N'Привет 4',   N'Γεια σου κόσμε! 4',  N'ППППП04',  '2002-04-18',  '04:40', '2004-04-14 15:21',  44.123456789,  44.1234567890123456,  433444666774.45,  0, 0x342E20544F42415341);
   )-"_asChar );



const std::string dropTableRawData(
   R"-(
         IF OBJECT_ID('dbo.rawdata', 'U') IS NOT NULL
            DROP TABLE dbo.rawdata;
   )-");

const std::string createTableRawdata(
   R"-(
         CREATE TABLE rawdata(
            id int IDENTITY(1,1) PRIMARY KEY,
            note varchar(10) NULL,
            rawdata text NULL,
            code int NULL
         );
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
}
