#include <tobasa/util_string.h>
#include <tobasa/crypt.h>
#include <tobasa/exception.h>
#include "tobasasql/util.h"

namespace tbs {
namespace util {

// Note: Taken from pgAdmin 3
bool needsQuoting(const std::string& value)
{
   if (isNumber(value))
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

std::string columnTypeClassToString(sql::TypeClass typeClass)
{
   std::string val;

   switch (typeClass)
   {
   case sql::TypeClass::numeric:
      val = "TypeClass_Numeric";
      break;
   case sql::TypeClass::boolean:
      val = "TypeClass_Boolean";
      break;
   case sql::TypeClass::string:
      val = "TypeClass_String";
      break;
   case sql::TypeClass::date:
      val = "TypeClass_Date";
      break;
   case sql::TypeClass::timestamp:
      val = "TypeClass_Timestamp";
      break;
   case sql::TypeClass::blob:
      val = "TypeClass_Blob";
      break;
   case sql::TypeClass::unknown:
      val = "TypeClass_Unknown";
      break;
   }
   return val;
}

std::string getConnectionString(const sql::conf::Database& dbOption, const std::string& securitySalt)
{
   std::string connString   = dbOption.connectionString;
   std::string encryptedPwd = dbOption.password;
   std::string clearPwd;

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


} // namespace util
} // namespace tbs