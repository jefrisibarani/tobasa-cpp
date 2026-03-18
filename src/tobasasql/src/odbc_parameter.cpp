
#include <tobasa/variant_helper.h>
#include <tobasa/logger.h>
#include "tobasasql/odbc_util.h"
#include "tobasasql/odbc_parameter.h"

namespace tbs {
namespace sql {

OdbcParameter::OdbcParameter()
{
   name          = "";
   number        = 0;
   direction     = SQL_PARAM_INPUT;
   valueType     = SQL_C_CHAR;
   type          = SQL_CHAR;
   columnSize    = 0;
   decimalDigits = 0;
   pValue        = nullptr;
   bufferLength  = 0;
   strLenIndPtr  = 0;
}

OdbcParameter::~OdbcParameter() {}

OdbcParameterCollection::OdbcParameterCollection(SQLHSTMT pStmt, short members)
{
   _pStmt = pStmt;

   collection.reserve(members);
   for (short i = 0; i < members; i++)
   {
      collection.emplace_back(std::make_shared<OdbcParameter>());
   }
}

void OdbcParameterCollection::prepare(const std::string& sql, const SqlParameterCollection& parameters)
{
   try
   {
      auto sqlcmd = tbs::util::utf8_to_odbcString(sql);
      SQLRETURN rc = SQLPrepare(_pStmt, (SQLTCHAR*)sqlcmd.c_str(), SQL_NTS);

      if (!SQL_SUCCEEDED(rc))
      {
         auto diag = statementDiagRecord(_pStmt, rc);
         throw tbs::SqlException(diag.message(), "OdbcParameterCollection");
      }

      for (unsigned int i = 0; i < parameters.size(); i++)
      {
         auto& parameter           = parameters.at(i);
         auto& odbcParam           = collection[i];
         SQLSMALLINT parameterType = sql::odbcTypeFromDataType(parameter->type());
         SQLSMALLINT direction     = sql::odbcParamDirectionFromParamDirection(parameter->direction());
         odbcParam->name           = parameter->name();
         odbcParam->number         = i + 1;
         odbcParam->direction      = direction;
         odbcParam->type           = parameterType;
         odbcParam->valueType      = SQL_C_TCHAR;
         odbcParam->pValue         = nullptr;
         odbcParam->bufferLength   = 0;
         odbcParam->strLenIndPtr   = 0;
         odbcParam->decimalDigits  = parameter->decimalDigits();

         // Note: https://docs.microsoft.com/en-us/sql/odbc/reference/appendixes/column-size?view=sql-server-ver15
         // The ColumnSize argument of SQLBindParameter is ignored for these data types:
         // bit, tinyint, smallint, integer, bigint, real, float, double, date, time
         // odbcParam->columnSize = 0;

         std::string errMsg = "Invalid variant type for " + dataTypeToString(parameter->type());   

         switch (parameterType)
         {
            case SQL_CHAR:
            case SQL_WVARCHAR:
            {
               if (std::holds_alternative<std::monostate>(parameter->value()))
               {
                  odbcParam->type         = SQL_WVARCHAR;
                  odbcParam->valueType    = SQL_C_TCHAR;
                  odbcParam->strLenIndPtr = SQL_NULL_DATA;
                  odbcParam->value        = std::wstring();
                  odbcParam->bufferLength = 0;
                  odbcParam->pValue       = NULL;
                  odbcParam->columnSize   = 0;
               }
               else
               {
                  const auto& strValue    = VariantHelper<>::value<std::string>(parameter->value(), errMsg);
                  auto strValueOdbc       = tbs::util::utf8_to_odbcString(strValue);
                  auto strValueOdbcLength = strValueOdbc.length();
                  odbcParam->type         = SQL_WVARCHAR;
                  odbcParam->valueType    = SQL_C_TCHAR;
                  odbcParam->strLenIndPtr = SQL_NTS;
                  odbcParam->value        = strValueOdbc;
                  odbcParam->bufferLength = 0;
                  odbcParam->columnSize   = strValueOdbcLength;
                  #if defined(_WIN32)
                  odbcParam->pValue       = (SQLPOINTER)(VariantHelper<>::value<std::wstring>(odbcParam->value, errMsg)).c_str();
                  #else
                  odbcParam->pValue       = (SQLPOINTER)(VariantHelper<>::value<std::string>(odbcParam->value, errMsg)).c_str();
                  #endif
               }
               break;
            }
            case SQL_TINYINT:
            case SQL_SMALLINT:
            case SQL_INTEGER:
            case SQL_BIGINT:
            {
               odbcParam->type = parameterType;

               if (std::holds_alternative<std::monostate>(parameter->value()))
               {
                  if (parameterType == SQL_TINYINT)
                     odbcParam->valueType  = SQL_C_STINYINT;
                  if (parameterType == SQL_SMALLINT)
                     odbcParam->valueType  = SQL_C_SSHORT;
                  if (parameterType == SQL_INTEGER)
                     odbcParam->valueType  = SQL_C_SLONG;
                  if (parameterType == SQL_BIGINT)
                     odbcParam->valueType  = SQL_C_SBIGINT;

                  odbcParam->strLenIndPtr = SQL_NULL_DATA;
                  odbcParam->value        = 0;
                  odbcParam->bufferLength = 0;
                  odbcParam->pValue       = NULL;
                  odbcParam->columnSize   = 0;
               }
               else if (std::holds_alternative<bool>(parameter->value()) &&  parameterType == SQL_TINYINT )
               {
                  // get and reset value for boolean parameter
                  bool value = std::get<bool>(parameter->value());
                  int8_t realVal = value ? (int8_t)1 : (int8_t)0;
                  parameter->value(realVal);

                  odbcParam->valueType = SQL_C_STINYINT;
                  odbcParam->value     = std::get<int8_t>(parameter->value());
                  odbcParam->pValue    = (SQLPOINTER) &(std::get<int8_t>(odbcParam->value));
               }
               else if (std::holds_alternative<int8_t>(parameter->value()))
               {
                  odbcParam->valueType = SQL_C_STINYINT;
                  odbcParam->value     = std::get<int8_t>(parameter->value());
                  odbcParam->pValue    = (SQLPOINTER) &(std::get<int8_t>(odbcParam->value));
               } 
               else if (std::holds_alternative<int16_t>(parameter->value()))
               {
                  odbcParam->valueType = SQL_C_SSHORT;
                  odbcParam->value     = std::get<int16_t>(parameter->value());
                  odbcParam->pValue    = (SQLPOINTER) &(std::get<int16_t>(odbcParam->value));
               } 
               else if (std::holds_alternative<int32_t>(parameter->value()))
               {
                  odbcParam->valueType = SQL_C_SLONG;
                  odbcParam->value     = VariantHelper<>::value<int32_t>(parameter->value(), errMsg);
                  odbcParam->pValue    = (SQLPOINTER) &(std::get<int32_t>(odbcParam->value));
               }
               else if (std::holds_alternative<int64_t>(parameter->value()))
               {
                  odbcParam->valueType = SQL_C_SBIGINT;
                  odbcParam->value     = VariantHelper<>::value<int64_t>(parameter->value(), errMsg);
                  odbcParam->pValue    = (SQLPOINTER) &(std::get<int64_t>(odbcParam->value));
               }
               else 
                  throw VariantException(errMsg);

               break;
            }

            case SQL_FLOAT:
            case SQL_REAL:
            {
               odbcParam->type = SQL_REAL;
               odbcParam->valueType = SQL_C_FLOAT;
               if (std::holds_alternative<std::monostate>(parameter->value()))
               {
                  odbcParam->strLenIndPtr = SQL_NULL_DATA;
                  odbcParam->value        = 0;
                  odbcParam->bufferLength = 0;
                  odbcParam->pValue       = NULL;
                  odbcParam->columnSize   = 0;
               }
               else
               {
                  odbcParam->value  = VariantHelper<>::value<float>(parameter->value(), errMsg);
                  odbcParam->pValue = (SQLPOINTER) &(VariantHelper<>::value<float>(odbcParam->value), errMsg);
               }
               break;
            }
            case SQL_DOUBLE:
            {
               odbcParam->type = SQL_DOUBLE;
               odbcParam->valueType = SQL_C_DOUBLE;
               if (std::holds_alternative<std::monostate>(parameter->value()))
               {
                  odbcParam->strLenIndPtr = SQL_NULL_DATA;
                  odbcParam->value        = 0;
                  odbcParam->bufferLength = 0;
                  odbcParam->pValue       = NULL;
                  odbcParam->columnSize   = 0;
               }
               else
               {
                  odbcParam->value  = VariantHelper<>::value<double>(parameter->value(), errMsg);
                  odbcParam->pValue = (SQLPOINTER) &(VariantHelper<>::value<double>(odbcParam->value, errMsg));
               }
               break;
            }
            case SQL_BIT:
            {
               odbcParam->type      = SQL_SMALLINT;
               odbcParam->valueType = SQL_C_SSHORT;
               if (std::holds_alternative<std::monostate>(parameter->value()))
               {
                  odbcParam->strLenIndPtr = SQL_NULL_DATA;
                  odbcParam->value        = 0;
                  odbcParam->bufferLength = 0;
                  odbcParam->pValue       = NULL;
                  odbcParam->columnSize   = 0;
               }
               else
               {
                  bool bvalue       = VariantHelper<>::value<bool>(parameter->value(), errMsg);
                  odbcParam->value  = (int)((bvalue == true) ? 1 : 0); // store as integer variant
                  odbcParam->pValue = (SQLPOINTER) &(VariantHelper<>::value<int>(odbcParam->value, errMsg));
               }
               break;
            }
            case SQL_VARBINARY:
            {
               // Parameter's data is encoded in Hexadecimal string
               #if 0
               // -------------------------------------------------------
               // Send binary data as characters in HEX Encoded string
               odbcParam->type         = SQL_VARBINARY;
               odbcParam->valueType    = SQL_C_CHAR;
               odbcParam->columnSize   = 0;
               odbcParam->bufferLength = 0;
               odbcParam->strLenIndPtr = SQL_NTS;
               odbcParam->value        = VariantHelper<>::value<std::string>(parameter->value(), errMsg);
               odbcParam->pValue       = (SQLPOINTER)(VariantHelper<>::value<std::string>(odbcParam->value, errMsg)).c_str();
               #endif
               #if 0
               // Send blob data in one segment, suitable for for smaller-length blobs
               odbcParam->type          = SQL_LONGVARBINARY;
               odbcParam->valueType     = SQL_C_BINARY;
               odbcParam->columnSize    = (SQLULEN) parameter->size();
               odbcParam->bufferLength  = 0;
               odbcParam->strLenIndPtr  = (SQLLEN) parameter->size();
               //odbcParam->value         = VariantHelper<>::value<std::string>(parameter->value(), errMsg);
               odbcParam->pValue        = (SQLPOINTER) *(parameter->valueBytePtr());
               #endif
               
               // -------------------------------------------------------
               // Send binary data with SQLPutData
               // Note: https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/sending-long-data?view=sql-server-ver15
               //       https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlputdata-function?view=sql-server-ver15
               // After SQLExecute or SQLExecDirect, if we got SQL_NEED_DATA, we use SQLPutData to send actual data
               
               odbcParam->type = SQL_LONGVARBINARY;
               odbcParam->valueType = SQL_C_BINARY;
               if (std::holds_alternative<std::monostate>(parameter->value()))
               {
                  odbcParam->strLenIndPtr = SQL_NULL_DATA;
                  odbcParam->value        = 0;
                  odbcParam->bufferLength = 0;
                  odbcParam->pValue       = NULL;
                  odbcParam->columnSize   = 0;
               }
               else
               {
                  SQLLEN lbytes            = (SDWORD)parameter->size();
                  odbcParam->value         = VariantHelper<>::value<std::string>(parameter->value(), errMsg);
                  odbcParam->columnSize    = (SQLULEN) parameter->size();
                  odbcParam->decimalDigits = 0;
                  // store parameter position, to be returned back in SQLParamData call
                  odbcParam->pValue        = (SQLPOINTER) odbcParam->number; // Note: gcc, warning pointer cast
                  odbcParam->bufferLength  = 0;
                  // use SQL_LEN_DATA_AT_EXEC() macro, so that  ODBC driver knows data is to be sent in segments.
                  odbcParam->strLenIndPtr  = SQL_LEN_DATA_AT_EXEC(lbytes); ;
               }

               break;
            }
            case SQL_NUMERIC:
            case SQL_TYPE_DATE:
            case SQL_TYPE_TIME:
            case SQL_TYPE_TIMESTAMP:
            {
               if (std::holds_alternative<std::monostate>(parameter->value()))
               {
                  //ret = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 255, 0, NULL, 0, &paramIndicator);
                  odbcParam->type         = SQL_CHAR;
                  odbcParam->valueType    = SQL_C_CHAR;
                  odbcParam->strLenIndPtr = SQL_NULL_DATA;
                  odbcParam->value        = 0;
                  odbcParam->bufferLength = 0;
                  odbcParam->pValue       = NULL;
                  odbcParam->columnSize   = 255;
               }
               else
               {
                  std::string strValue    = VariantHelper<>::value<std::string>(parameter->value(), errMsg);
                  long strValueLen        = static_cast<long>(strValue.length());

                  odbcParam->type         = SQL_CHAR;
                  odbcParam->valueType    = SQL_C_CHAR;
                  odbcParam->strLenIndPtr = SQL_NTS;
                  odbcParam->value        = strValue;  // store as std::string variant
                  odbcParam->bufferLength = 0;
                  odbcParam->pValue       = (SQLPOINTER)(VariantHelper<>::value<std::string>(odbcParam->value, errMsg)).c_str();
                  odbcParam->columnSize   = strValueLen;
               }

               break;
            }
            default:
               throw TypeException("Unsupported type " + dataTypeToString(parameter->type()) );
               break;
         }
      } // end for loop
   }
   catch (const tbs::VariantException &ex)
   {
      throw tbs::SqlException(tbsfmt::format("prepare, {}", ex.what()),"OdbcParameterCollection");
   }
   catch (const tbs::TypeException &ex)
   {
      throw tbs::SqlException(tbsfmt::format("prepare, {}", ex.what()),"OdbcParameterCollection");
   }
}

void OdbcParameterCollection::bindParameter()
{
   for (unsigned int i = 0; i < collection.size(); i++)
   {
      SQLRETURN rc;
      auto odbcParam = collection[i];

      rc = SQLBindParameter(
               _pStmt,                       // SQLHSTMT       StatementHandle    [Input]
               odbcParam->number,            // SQLUSMALLINT   ParameterNumber    [Input]
               odbcParam->direction,         // SQLSMALLINT    InputOutputType    [Input]
               odbcParam->valueType,         // SQLSMALLINT    ValueType          [Input]  Parameter is a string
               odbcParam->type,              // SQLSMALLINT    ParameterType      [Input]  Destination type in database table
               odbcParam->columnSize,        // SQLULEN        ColumnSize         [Input]
               odbcParam->decimalDigits,     // SQLSMALLINT    DecimalDigits      [Input]
               odbcParam->pValue,            // SQLPOINTER     ParameterValuePtr  [Deferred Input]
               odbcParam->bufferLength,      // SQLLEN         BufferLength       [Input/Output]
               &(odbcParam->strLenIndPtr));  // SQLLEN         StrLen_or_IndPtr   [Deferred Input]

      if (rc == SQL_SUCCESS)
      {
         //Logger::logD("SQLBindParameter param no: {} name: {} success.", odbcParam->name, odbcParam->number);
      }
      else if (rc == SQL_SUCCESS_WITH_INFO)
      {
         auto diag = statementDiagRecord(_pStmt, rc);
         Logger::logD(diag.message(), "odbcconn");
      }
      else if (!SQL_SUCCEEDED(rc))
      {
         auto diag = statementDiagRecord(_pStmt, rc);
         throw tbs::SqlException(tbsfmt::format("prepare, {}", diag.message()), "OdbcParameterCollection");
      }
      else
      {
         Logger::logE("SQLBindParameter param no: {} name: {} unknow error occured", odbcParam->name, odbcParam->number);
         auto diag = statementDiagRecord(_pStmt, rc);
         throw tbs::SqlException(tbsfmt::format("prepare, {}", diag.message()), "OdbcParameterCollection");
      }
   }
}

std::shared_ptr<OdbcParameter> OdbcParameterCollection::getParam(int pos)
{
    return collection[pos];
}


} // namespace sql
} // namespace tbs