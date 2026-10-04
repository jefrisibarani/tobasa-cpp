#pragma once

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */

/// SQL Backend type.
enum class BackendType
{
   pgsql,   ///< PostgreSQL
   sqlite,  ///< SQLite
   adodb,   ///< ADO Database
   odbc,    ///< ODBC Database
   mysql,   ///< MySQL
   unknown  ///< Unknown
};

/// SQL Connection status.
enum class ConnectionStatus
{
   bad,
   ok,
   refused,
   dnsError,
   aborted,
   broken
};

/// SQL Result status.
enum class ResultStatus
{
   unknown,

   /// Successful completion of a command returning no data.
   tuplesOk,

   ///Successful completion of a command returning data (such as a SELECT or SHOW)
   commandOk
};


// Note
// https://docs.microsoft.com/en-us/sql/odbc/reference/appendixes/sql-data-types?view=sql-server-ver15
// https://www.postgresql.org/docs/9.5/datatype.html
// https://docs.microsoft.com/en-us/sql/t-sql/data-types/float-and-real-transact-sql?view=sql-server-ver15
// https://docs.microsoft.com/en-us/dotnet/api/system.data.sqldbtype?view=net-5.0#System_Data_SqlDbType_Decimal
// https://dev.mysql.com/doc/refman/8.0/en/data-types.html


/** 
 * SQL data types mapping (portable C++ fixed-width equivalents)
 */
enum class DataType
{
   /** 
    * \brief 8-bit unsigned integer.
    * Range: 0 to 255 (1 byte storage)
    * Alias : byte, tinyint
    * C++   : uint8_t
    */
   tinyint,

   /** 
    * \brief Signed 16-bit integer.
    * Range: -32768 to +32767 (2 bytes storage)
    * Alias : small integer
    * C++   : int16_t
    */
   smallint,

   /** 
    * \brief Signed 32-bit integer.
    * Range: -2147483648 to +2147483647 (4 bytes storage)
    * Alias : integer
    * C++   : int32_t
    */
   integer,

   /** 
    * \brief Signed 64-bit integer.
    * Range: -9223372036854775808 to +9223372036854775807 (8 bytes storage)
    * Alias : big integer
    * C++   : int64_t
    */
   bigint,

   /** 
    * \brief Exact numeric type (arbitrary precision).
    * Alias : decimal, numeric
    * C++   : std::string (text representation)
    */
   numeric,

   /** 
    * \brief 32-bit floating-point (single precision).
    * Alias : real, float, float4, float(1-24), single (in .NET)
    * C++   : float
    */
   float4,

   /** 
    * \brief 64-bit floating-point (double precision).
    * Alias : double, double precision, float(25-53)
    * C++   : double
    * MySQL : default treat REAL as DOUBLE
    */
   float8,

   /** 
    * \brief Logical Boolean (true/false).
    * C++   : bool
    * MySQL : tinyint(1)
    * MSSQL : bit
    * PGSQL : boolean
    */
   boolean,

   /** 
    * \brief Fixed-length character string.
    * C++   : std::string
    */
   character,

   /** 
    * \brief Variable-length character string.
    * C++   : std::string
    */
   varchar,

   /** 
    * \brief Text (large unbounded character data).
    * C++   : std::string
    */
   text,

   /** 
    * \brief Calendar date (year, month, day).
    * C++   : std::string (ISO-8601: "YYYY-MM-DD")
    */
   date,

   /** 
    * \brief Time of day (no time zone).
    * C++   : std::string (ISO-8601: "HH:MM:SS")
    */
   time,

   /** 
    * \brief Date and time (no time zone).
    * C++   : std::string (ISO-8601: "YYYY-MM-DD HH:MM:SS")
    * MySQL : datetime
    * MSSQL : datetime
    * PGSQL : timestamp
    */
   timestamp,

   /** 
    * \brief Binary, blob data.
    * C++   : std::vector<uint8_t>
    */
   varbinary,

   /** 
    * \brief Bit-string like "10101010".
    * C++   : std::vector<uint8_t>
    * MySQL : BIT
    * PGSQL : BIT VARBIT
    */
   varbit,

   /** 
    * \brief Unknown / not mapped type.
    */
   unknown
};

/// SQL Column type class.
enum class TypeClass
{
   numeric,
   boolean,
   string,
   date,
   timestamp,
   blob,
   unknown
};

/// Parameter direction.
enum class ParameterDirection
{
   input,
   output,
   inputOutput,
   returnValue,
   unknown
};


enum class ParameterStyle {
   native, // SQL already uses DB-native param style
   named   // SQL uses :name placeholders and must be parsed
};

const long FIELD_SIZE_UNKNOWN = -1;

const std::string NULLSTR = "null";
const std::string BLOBSTR = "blob";
const std::string BITSTR  = "bit";

/** @}*/

} // namespace sql
} // namespace tbs