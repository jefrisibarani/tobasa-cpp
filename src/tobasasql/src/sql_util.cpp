#include <tobasa/util_string.h>
#include <tobasa/crypt.h>
#include <tobasa/exception.h>
#include "tobasasql/sql_util.h"

namespace tbs {
namespace sql {

// Note: Taken from pgAdmin 3
bool needsQuoting(const std::string& value)
{
   if (util::isNumber(value))
      return true;
   else
   {
      // certain types should not be quoted even though it contains a space. Evilness.
      std::string valNoArray;
      if (value.find("[]", value.length() - 2) != std::string::npos) {
         valNoArray = value.substr(0, value.length() - 2);
      }
      else {
         valNoArray = value;
      }

      auto ValNoArrayLowCase = util::toLower(valNoArray);
      // PosgtreSql types
      if (ValNoArrayLowCase == "character varying"           ||
          ValNoArrayLowCase == "\"char\""                    ||
          ValNoArrayLowCase == "bit varying"                 ||
          ValNoArrayLowCase == "double precision"            ||
          ValNoArrayLowCase == "timestamp without time zone" ||
          ValNoArrayLowCase == "timestamp with time zone"    ||
          ValNoArrayLowCase == "time without time zone"      ||
          ValNoArrayLowCase == "time with time zone"         ||
          ValNoArrayLowCase == "\"trigger\""                 ||
          ValNoArrayLowCase == "\"unknown\"") 
      {
         return false;
      }

      int pos = 0;
      while (pos < (int) valNoArray.length())
      {
         char c = valNoArray.at(pos);

         if ( ! ((pos > 0) && (c >= '0' && c <= '9')) &&
              ! (c >= 'a' && c <= 'z') &&
              ! (c == '_'))
         {
            return true;
         }
         pos++;
      }
   }

   return false;
}

std::string quoteIdent(const std::string& value)
{
   if (value.length() == 0)
      return value;

   std::string result = value;
   if (needsQuoting(result))
      return "\"" + result + "\"";
   else
      return result;
}

std::string backendTypeToString(BackendType type)
{
   switch (type)
   {
   case BackendType::pgsql:
      return "PostgreSQL";
   case BackendType::sqlite:
      return "SQLite";
   case BackendType::adodb:
      return "ADO Database";
   case BackendType::odbc:
      return "ODBC Database";
   case BackendType::mysql:
      return "MySQL";
   default:
      return "unknown";
   }
}

BackendType backendTypeFromString(const std::string& type)
{
   if (type == "PGSQL")
      return BackendType::pgsql;
   else if (type == "SQLITE")
      return BackendType::sqlite;
   else if (type == "ADODB")
      return BackendType::adodb;
   else if (type == "ODBC")
      return BackendType::odbc;
   else if (type == "MYSQL")
      return BackendType::mysql;
   else 
      return BackendType::unknown;
}

std::string dataTypeToString(DataType type)
{
   switch (type)
   {
   case DataType::tinyint:
      return "tinyint";
   case DataType::smallint:
      return "smallint";
   case DataType::integer:
      return "integer";
   case DataType::bigint:
      return "bigint";
   case DataType::numeric:
      return "numeric";
   case DataType::float4:
      return "float4";
   case DataType::float8:
      return "float8";
   case DataType::boolean:
      return "boolean";
   case DataType::character:
      return "character";
   case DataType::varchar:
      return "varchar";
   case DataType::text:
      return "text";
   case DataType::date:
      return "date";
   case DataType::time:
      return "time";
   case DataType::timestamp:
      return "timestamp";
   case DataType::varbinary:
      return "varbinary";
   case DataType::varbit:
      return "varbit";
   default:
      return "unknown";
   }
}

std::string columnTypeClassToString(TypeClass typeClass)
{
   switch (typeClass)
   {
   case TypeClass::numeric:
      return "TypeClass_Numeric";
   case TypeClass::boolean:
      return "TypeClass_Boolean";
   case TypeClass::string:
      return "TypeClass_String";
   case TypeClass::date:
      return "TypeClass_Date";
   case TypeClass::timestamp:
      return "TypeClass_Timestamp";
   case TypeClass::blob:
      return "TypeClass_Blob";
   case TypeClass::unknown:
      return "TypeClass_Unknown";
   default:
      return "";
   }
}

std::string getConnectionString(const conf::Database& dbOption, const std::string& securitySalt)
{
   std::string connString   = dbOption.connectionString;
   std::string encryptedPwd = dbOption.password;
   std::string clearPwd;

   if (securitySalt.empty())
      clearPwd = dbOption.password;
   else
      clearPwd = crypt::passwordDecrypt(encryptedPwd, securitySalt);

   using namespace sql;
   switch (dbOption.dbDriver)
   {
   case BackendType::pgsql:
      connString += " password=" + clearPwd;
      break;
   case BackendType::sqlite:
      connString += "Password=" + clearPwd + ";";
      break;
   case BackendType::odbc:
      connString += "Pwd=" + clearPwd + ";";
      break;
#if defined(_MSC_VER)
   case BackendType::adodb:
      connString += "Pwd=" + clearPwd + ";";
      break;
#endif
   case BackendType::mysql:
      connString += "Password=" + clearPwd + ";";
      break;
   default:
      throw AppException("Unknown SQL driver type");
      break;
   }

   return connString;
}

DataType dataTypeFromDeclaredType(const std::string& colType)
{
   std::string typeStr = util::toUpper(colType);

   if (     util::startsWith(typeStr, "TINYINT"))
      return DataType::tinyint;
   else if (util::startsWith(typeStr, "SMALLINT" ))
      return DataType::smallint;
   else if (util::startsWith(typeStr, "INT")
         || util::startsWith(typeStr, "MEDIUMINT")
         || util::startsWith(typeStr, "INTEGER") ) 
   {
      return DataType::integer;
   }
   else if (util::startsWith(typeStr, "BIGINT") || util::startsWith(typeStr, "BIG INTEGER"))
      return DataType::bigint;
   else if (util::startsWith(typeStr, "BOOL"))
      return DataType::boolean;
   else if (util::startsWith(typeStr, "CHAR") && !util::contains(typeStr,"VARYING"))
      return DataType::character;
   else if (util::startsWith(typeStr, "CHARACTER VARYING")
         || util::startsWith(typeStr, "VARCHAR")
         || util::startsWith(typeStr, "STRING")) 
   {
      return DataType::varchar;
   }
   else if (util::startsWith(typeStr, "TEXT"))
      return DataType::text;
   else if (util::startsWith(typeStr, "DOUBLE"))
      return DataType::float8;
   else if (util::startsWith(typeStr, "FLOAT"))
      return DataType::float4;
   else if (util::startsWith(typeStr, "SINGLE")  || util::startsWith(typeStr, "REAL"))
      return DataType::float4;
   else if (util::startsWith(typeStr, "NUMERIC") || util::startsWith(typeStr, "DECIMAL"))
      return DataType::numeric;
   else if (util::startsWith(typeStr, "BLOB"))
      return DataType::varbinary;
   else if (typeStr != "DATETIME" && util::startsWith(typeStr, "DATE"))
      return DataType::date;
   else if (typeStr != "TIMESTAMP" && util::startsWith(typeStr, "TIME"))
      return DataType::time;
   else if (util::startsWith(typeStr, "DATETIME"))
      return DataType::timestamp;
   else if (util::startsWith(typeStr, "TIMESTAMP"))
      return DataType::timestamp;
   else if (util::startsWith(typeStr, "BIT") || util::startsWith(typeStr, "VARBIT"))
      return DataType::varbit;
   else 
   {
      return DataType::unknown;
   }
}

TypeClass typeClassFromDataType(DataType type)
{
   switch (type)
   {
   case DataType::tinyint:
   case DataType::smallint:
   case DataType::integer:
   case DataType::numeric:
   case DataType::float4:
   case DataType::float8:
      return TypeClass::numeric;
   case DataType::boolean:
      return TypeClass::boolean;
   case DataType::character:
   case DataType::varchar:
   case DataType::text:
      return TypeClass::string;
   case DataType::varbinary:
   case DataType::varbit:
      return TypeClass::blob;
   case DataType::date:
   case DataType::time:
   case DataType::timestamp:
      return TypeClass::date;
   default:
      throw TypeException("Invalid SQLite data type conversion to TypeClass", "SQLUtil");
   }
}

} // namespace sql
} // namespace tbs