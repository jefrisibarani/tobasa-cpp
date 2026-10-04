#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#include <tobasa/exception.h>
#include <tobasa/crypt.h>
#include "tobasasql/sql_dataset.h"
#include "tobasasql/adodb_util.h"
#include "tobasasql/adodb_result.h"
#include "tobasasql/adodb_connection.h"
#include "tobasasql/adodb_command.h"
#include <atlsafe.h>

namespace tbs {
namespace sql {

AdodbCommand::AdodbCommand(AdodbConnection* conn)
   : _pConn(conn)
   , _pAdoConn(nullptr)
   , _pAdoCommand(nullptr)
   , _affectedRows(-1)
{
   if (_pConn)
      _pAdoConn = _pConn->nativeConnection();

   notifierSource = "AdodbCommand";
}

bool AdodbCommand::query(const std::string& sql, const AdoParameterCollection& parameters)
{
   if (!prepare(sql))
      return false;

   return bind(parameters);
}

bool AdodbCommand::prepare(const std::string& sql)
{
   if (_pAdoConn == nullptr)
      throw SqlException("Invalid connection object", "AdodbCommand");

   close();

   try
   {
      HRESULT hr = _pAdoCommand.CreateInstance(__uuidof(ADODB::Command));
      if (FAILED(hr) || _pAdoCommand == nullptr)
         _com_issue_error(FAILED(hr) ? hr : E_FAIL);

      _sql = sql;
      _pAdoCommand->CommandText = sql::utf8_to_bstr_t(sql);
      _pAdoCommand->CommandType = ADODB::adCmdText;
      _pAdoCommand->ActiveConnection = _pAdoConn;

      return true;
   }
   catch (...)
   {
      close();
      throw;
   }
}

bool AdodbCommand::bind(const AdoParameterCollection& parameters)
{
   try
   {
      if (_pAdoConn == nullptr)
         throw SqlException("Invalid connection object", "AdodbCommand");

      if (_pAdoCommand == nullptr)
         throw SqlException("Invalid command object", "AdodbCommand");

      onNotifyTrace(_pConn->logId() + "AdodbCommand, Binding paramaters" );

      if (_parameters.empty() && parameters.size()>0)
      {
         // Populate Native ADO Parameter collection once, and cache it
         for (const auto& param : parameters)
         {
         auto nativeParam = createParameter(*param);
         _pAdoCommand->Parameters->Append(nativeParam);
         _parameters.push_back(nativeParam);
         }
      }

      // Update native ado parameter with the actual parameter
      for (size_t i = 0; i < parameters.size(); i++)
      {
         auto nativeParameter = createParameter(*parameters.at(i));
         _parameters[i]->Value = nativeParameter->Value;
      }

      return true;
   }
   catch (_com_error& error)
   {
      _bstr_t description = error.Description();
      const char* message = description.length() > 0 ? static_cast<const char*>(description) : "ADO execute failed";
      throw SqlException(message, "AdodbCommand");
   }
}

void AdodbCommand::reset()
{
   if (_pAdoConn == nullptr)
      throw SqlException("Invalid connection object", "AdodbCommand");

   _affectedRows = -1;
}

void AdodbCommand::close()
{
   _parameters.clear();
   _pAdoCommand = nullptr;
   _sql.clear();
   _affectedRows = -1;
}

int AdodbCommand::execute()
{
   if (_pAdoConn == nullptr)
      throw SqlException("Invalid connection object", "AdodbCommand");

   if (_pAdoCommand == nullptr)
      throw SqlException("Invalid command object", "AdodbCommand");

   if (_pConn->logSqlQuery())
      onNotifyDebug(_pConn->logId() + tbsfmt::format("execute: {}", _sql));

   try
   {
      _variant_t recordsAffected;
      ADODB::_RecordsetPtr result = _pAdoCommand->Execute(
         &recordsAffected, nullptr, ADODB::adCmdText | ADODB::adExecuteNoRecords);

      if (recordsAffected.vt == VT_I4 || recordsAffected.vt == VT_INT)
         _affectedRows = recordsAffected.iVal < 0 ? 0 : recordsAffected.iVal;
      else
         _affectedRows = 0;

      if (_pConn->logExecuteStatus()) 
         onNotifyDebug(_pConn->logId() + tbsfmt::format("SQL command executed successfully, affectedRows: {}", _affectedRows));

      return static_cast<int>(_affectedRows);
   }
   catch (_com_error& error)
   {
      _bstr_t description = error.Description();
      const char* message = description.length() > 0 ? static_cast<const char*>(description) : "ADO execute failed";
      throw SqlException(message, "AdodbCommand");
   }
}

std::string AdodbCommand::executeScalar()
{
   if (_pAdoConn == nullptr)
      throw SqlException("Invalid connection object", "AdodbCommand");

   if (_pAdoCommand == nullptr)
      throw SqlException("Invalid command object", "AdodbCommand");

   if (_pConn->logSqlQuery())
      onNotifyDebug(_pConn->logId() + tbsfmt::format("executeScalar: {}", _sql));

   try
   {
      _affectedRows = 0;
      _variant_t recordsAffected;
      ADODB::_RecordsetPtr recordset = _pAdoCommand->Execute(&recordsAffected, nullptr, ADODB::adCmdText);
      if (recordset == nullptr || recordset->State == ADODB::adStateClosed)
         return "";

      if (recordsAffected.vt == VT_I4 || recordsAffected.vt == VT_INT)
         _affectedRows = recordsAffected.iVal < 0 ? 0 : recordsAffected.iVal;

      if (_pConn->logExecuteStatus())
         onNotifyDebug(_pConn->logId() + tbsfmt::format("Scalar query executed successfully with affected row: {}", _affectedRows));

      std::string result;
      long nRows = 0;
      while (!recordset->EndOfFile)
      {
         if (nRows == 0)
         {
            // We only interested on first record
            ADODB::FieldsPtr pFldLoop = nullptr;
            _variant_t vtIndex;
            vtIndex.vt    = VT_I2;                 // set vtIndex to save 2 byte int
            pFldLoop     = recordset->GetFields(); // get Fields pointer
            vtIndex.iVal = 0;

            _variant_t fieldValue = pFldLoop->GetItem(vtIndex)->Value;
            if (fieldValue.vt == VT_NULL)
            {
               if (_pConn->logExecuteStatus())
                  onNotifyDebug(_pConn->logId() + "Scalar query returned SQL NULL");

               result = sql::NULLSTR;
            }
            else
            {
               _bstr_t result_ = (_bstr_t) fieldValue;
               result = sql::utf8_from_bstr_t(result_);
            }
         }
         nRows++;
         recordset->MoveNext();
      }

      if (_pConn->logExecuteStatus())
      {
         if (nRows < 0)
            onNotifyDebug(_pConn->logId() + "Could not determine the number of records");
         else if (nRows == 0)
            onNotifyDebug(_pConn->logId() + "Scalar query returned no row");
         else if (nRows > 1)
            onNotifyDebug(_pConn->logId() + "Scalar query returned more than one row");
      }

      // Cleaning up COM Object
      if (recordset->State == ADODB::adStateOpen) {
         recordset->Close();
      }

      recordset = nullptr;
      return result;
   }
   catch (_com_error& error)
   {
      _bstr_t description = error.Description();
      const char* message = description.length() > 0 ? static_cast<const char*>(description) : "ADO execute failed";
      throw SqlException(message, "AdodbCommand");
   }

   return "";
}

AdodbCommand::DataSetPtr AdodbCommand::executeResult()
{
   if (_pAdoConn == nullptr)
      throw SqlException("Invalid connection object", "AdodbCommand");

   if (_pAdoCommand == nullptr)
      throw SqlException("Invalid command object", "AdodbCommand");

   if (_pConn->logSqlQuery())
      onNotifyDebug(_pConn->logId() + tbsfmt::format("executeResult: {}", _sql));

   try
   {
      _affectedRows = 0;
      _variant_t recordsAffected;
      ADODB::_RecordsetPtr recordset = _pAdoCommand->Execute(&recordsAffected, nullptr, ADODB::adCmdText);
      auto result = std::make_shared<DataSet<VariantType>>();

      if (recordset == nullptr || recordset->State == ADODB::adStateClosed)
         return result;

      if (recordsAffected.vt == VT_I4 || recordsAffected.vt == VT_INT)
         _affectedRows = recordsAffected.iVal < 0 ? 0 : recordsAffected.iVal;

      ADODB::CursorTypeEnum cursorType = recordset->GetCursorType();
      
      auto nRows = recordset->GetRecordCount();  // Count the correct total records
      result->totalColumns(static_cast<int>(recordset->Fields->GetCount()));
      
      if (_pConn->logExecuteStatus()) 
         onNotifyDebug(_pConn->logId() + tbsfmt::format("SQL command executed successfully, affectedRows: {}", _affectedRows));

      while (!recordset->EndOfFile)
      {
         auto& row = result->addRow();

         ADODB::FieldsPtr pOneRow = recordset->Fields;

         for (int col = 0; col < result->totalColumns(); col++)
         {
            _variant_t vFieldPos;
            vFieldPos.vt = VT_I2;
            vFieldPos.iVal = col;

            ADODB::FieldPtr field = pOneRow->GetItem(vFieldPos);

            /*
            Note: With Provider=SQLNCLI11, com_error occured if sql data type is date and DataTypeCompatibility=80  not set in connection string
            https://stackoverflow.com/questions/38662438/using-sql-server-datetime2-with-tadoquery-open
            https://stackoverflow.com/a/38664425

            | SQL Server data type | SQLOLEDB        | SQLNCLI            | SQLNCLI w/DataTypeCompatibilyt=80 |
            |----------------------|-----------------|--------------------|-----------------------------------|
            | Xml                  | adLongVarWChar  | 141 (DBTYPE_XML)   | adLongVarChar                     |
            | datetime             | adDBTimeStamp   | adDBTimeStamp      | adDBTimeStamp                     |
            | datetime2            | adVarWChar      | adDBTimeStamp      | adVarWChar                        |
            | date                 | adVarWChar      | adDBDate           | adVarWChar                        |
            | time                 | adVarWChar      | 145 (unknown)      | adVarWChar                        |
            | UDT                  |                 | 132 (DBTYPE_UDT)   | adVarBinary (documented,untested) |
            | varchar(max)         | adLongVarChar   | adLongVarChar      | adLongVarChar                     |
            | nvarchar(max)        | adLongVarWChar  | adLongVarWChar     | adLongVarWChar                    |
            | varbinary(max)       | adLongVarBinary | adLongVarBinary    | adLongVarBinary                   |
            | timestamp            | adBinary        | adBinary           | adBinary                          |
            */

            // NOTE_JEFRI: bigint returned as DECIMAL

            const _variant_t vValue = field->GetValue();
            row.emplace_back(vValue);
         }

         recordset->MoveNext();
      }

      return result;
   }
   catch (_com_error& error)
   {
      _bstr_t description = error.Description();
      const char* message = description.length() > 0 ? static_cast<const char*>(description) : "ADO execute failed";
      throw SqlException(message, "AdodbCommand");
   }
}

AdodbResult AdodbCommand::executeSqlResult()
{
   if (_pAdoConn == nullptr)
      throw SqlException("Invalid connection object", "AdodbCommand");

   AdodbResult result;
   result.connection(_pConn);
   result.runPreparedQuery(*this);

   return std::move(result);
}

ADODB::_RecordsetPtr AdodbCommand::executeRecordsetPtr()
{
   if (_pAdoConn == nullptr)
      throw SqlException("Invalid connection object", "AdodbCommand");
      
   if (_pAdoCommand == nullptr)
      throw SqlException("Invalid command object", "AdodbCommand");

   if (_pConn->logSqlQuery())
      onNotifyDebug(_pConn->logId() + tbsfmt::format("executeRecordsetPtr: {}", _sql));

   try
   {
      _affectedRows = 0;
      _variant_t recordsAffected;

      ADODB::_RecordsetPtr recordset = _pAdoCommand->Execute(&recordsAffected, nullptr, ADODB::adCmdText);

      if (recordsAffected.vt == VT_I4 || recordsAffected.vt == VT_INT)
         _affectedRows = recordsAffected.iVal < 0 ? 0 : recordsAffected.iVal;

      ADODB::CursorTypeEnum cursorType = recordset->GetCursorType();
      
      //auto nRows = recordset->GetRecordCount();  // Count the correct total records
      if (_pConn->logExecuteStatus()) 
         onNotifyDebug(_pConn->logId() + tbsfmt::format("SQL command executed successfully, affectedRows: {}", _affectedRows));

      return recordset;
   }
   catch (_com_error& error)
   {
      _bstr_t description = error.Description();
      const char* message = description.length() > 0 ? static_cast<const char*>(description) : "ADO execute failed";
      throw SqlException(message, "AdodbCommand");
   }
}

ADODB::_ParameterPtr AdodbCommand::createParameter( AdoParameter& param )
{
   ADODB::_ParameterPtr pParameter = nullptr;

   HRESULT hr = pParameter.CreateInstance(__uuidof(ADODB::Parameter));
   if (hr != S_OK) {
      _com_issue_error(hr);
   }

   _bstr_t paramName = sql::utf8_to_bstr_t(param.name());
   ADODB::DataTypeEnum paramType = adoDataTypeFromDataType(param.type());

   // Note:
   // for date, time and timestamp, adoDataTypeToDataType does not convert to characters
   // so we do it here. We send date, time and timestamp as characters
   // https://docs.microsoft.com/en-us/dotnet/framework/data/adonet/sql/date-and-time-data
   // https://docs.microsoft.com/en-us/dotnet/framework/data/adonet/configuring-parameters-and-parameter-data-types
   // https://stackoverflow.com/questions/38662438/using-sql-server-datetime2-with-tadoquery-open
   // https://stackoverflow.com/questions/60695196/how-to-parameterize-12-30-1899-to-sql-server-native-client-when-datatypecompatil
   // https://stackoverflow.com/questions/38662438/using-sql-server-datetime2-with-tadoquery-open
   
   if (  paramType == ADODB::adDate
      || paramType == ADODB::adDBTime
      || paramType == ADODB::adDBTimeStamp)
   {
      paramType = ADODB::adVarWChar;
   }

   long paramSize = static_cast<long>(param.size());

   // for these types, we set paramSize to -1
   if (  param.type() == DataType::character
      || param.type() == DataType::varchar
      || param.type() == DataType::text
      || param.type() == DataType::date
      || param.type() == DataType::time
      || param.type() == DataType::timestamp)
   {
      paramSize = -1;
   }

   // Send numeric or decimal type as string to database
   if (param.type() == DataType::numeric)
   {
      paramType = ADODB::adVarWChar;
      paramSize = -1;
   }

   _variant_t paramValue;

   if (std::holds_alternative<std::monostate>(param.value()))
   {
      paramValue.vt = VT_NULL;
      paramSize  = 0;
   }
   else if (paramType == ADODB::adVarBinary)
   {
      paramSize = -1;

      try
      {
         auto bytes = param.valueBinaryPtr();
         uint8_t* pData = *bytes;
         size_t len = param.dataSize();

         if (len > static_cast<size_t>(LONG_MAX))
            throw std::length_error("binary parameter is too large for SAFEARRAY");

         // Create a safe array storing BYTEs
         const LONG count = static_cast<LONG>(len);
         CComSafeArray<BYTE> sa(count);

         // Fill the safe array with some data
         for (LONG i=0; i<count; i++)
         {
            sa[i] = pData[static_cast<size_t>(i)];
         }

         paramValue.parray = sa.Detach();

         // 8209 _variant_t vt 8209 sql server binary data type
         // Note: VT_ARRAY | VT_UI1 = 0x2000 | 0x0011 = 0x2011 = 8209
         // One-dimensional SAFEARRAY whose element type is VT_UI1 (BYTE)
         paramValue.vt = VT_ARRAY | VT_UI1;
      }
      catch (const CAtlException& e)
      {
         (void) e;
         throw std::exception("error creating SafeArray");
      }
      catch (const std::exception& e)
      {
         throw e;
      }
      
   }
   else 
   {
      paramValue = VariantHelper::toNativeVariant( param.value() );
   }

   ADODB::ParameterDirectionEnum paramDirection = adoParamDirectionFromParamDirection( param.direction() );
   pParameter->Name      = paramName;
   pParameter->Type      = paramType;
   pParameter->Size      = paramSize;
   pParameter->Direction = paramDirection;

   pParameter->Value = paramValue;

   return pParameter;
}

std::string AdodbCommand::getScalarResult(ADODB::_RecordsetPtr recordset, long& totalRows)
{
   if (recordset == nullptr)
      throw std::runtime_error("invalid recordset pointer");

   long nRows0 = recordset->GetRecordCount();
   
   std::string result;

   // loop through the result to get total rows
   long nRows = 0;

   while (!recordset->EndOfFile)
   {
      if (nRows == 0)
      {
         // We only interested on first record
         ADODB::FieldsPtr pFldLoop = nullptr;
         _variant_t vtIndex;
         vtIndex.vt = VT_I2;           // set vtIndex to save 2 byte int
         pFldLoop = recordset->GetFields(); // get Fields pointer
         vtIndex.iVal = 0;

         _variant_t fieldValue = pFldLoop->GetItem(vtIndex)->Value;
         if (fieldValue.vt == VT_NULL)
         {
            //onNotifyDebug(_pConn->logId() + "Scalar query returned SQL NULL");
            result = sql::NULLSTR;
         }
         else
         {
            _bstr_t result_ = (_bstr_t) fieldValue;
            result = sql::utf8_from_bstr_t(result_);
         }
      }
      nRows++;
      recordset->MoveNext();
   }

   totalRows = nRows;
   return result;
}

ADODB::_CommandPtr AdodbCommand::createNativeCommand(const std::string& sql, const AdoParameterCollection& parameters)
{
   ADODB::_CommandPtr pCommand = nullptr;

   try
   {
      HRESULT hr = pCommand.CreateInstance(__uuidof(ADODB::Command));
      if (hr != S_OK) {
         _com_issue_error(hr);
      }

      _bstr_t sqlCmdBW = sql::utf8_to_bstr_t(sql);
      pCommand->CommandText = sqlCmdBW; /*(_bstr_t)sql.c_str()*/
      pCommand->CommandType = ADODB::adCmdText;

      for (size_t i = 0; i < parameters.size(); i++)
      {
         auto param = parameters.at(i);
         auto parameter = createParameter(*param);
         pCommand->Parameters->Append(parameter);
      }

      pCommand->ActiveConnection = _pAdoConn;

      return pCommand;
   }
   catch (_com_error& e)
   {
      pCommand = nullptr;
      throw e;
   }
   catch (std::exception& e)
   {
      throw e;
   }

   return nullptr;
}

/*
void AdodbCommand::clearNativeParameters()
{
   if (_pAdoCommand == nullptr)
      return;

   while (_pAdoCommand->Parameters->Count > 0)
   {
      _pAdoCommand->Parameters->Delete(
         _pAdoCommand->Parameters->Count - 1);
   }
}
*/

// --------------------------------------------------------------------------
/*
ADODB::_RecordsetPtr AdodbCommand::executeRecordsetPtr(const std::string& sql, const AdoParameterCollection& parameters)
{
   ADODB::_CommandPtr pCommand = nullptr;
   ADODB::_RecordsetPtr pResult = nullptr;

   try
   {
      Logger::logD("[sql] [AdodbCommand] executing query, total parameter: {}", parameters.size());
      pCommand = createNativeCommand(sql, parameters);
      pResult  = pCommand->Execute(nullptr, nullptr, ADODB::adCmdText);

      return pResult;
   }
   catch(const _com_error& e)
   {
      // pCommand is a _com_ptr_t, before it is going out of scope, detach its internal interface
      // otherwise its destrutor will throw
      auto ifc = pCommand.Detach();
      ifc = nullptr;

      throw e;
   }
   catch(const std::exception& e)
   {
      throw e;
   }

   return nullptr;
}
*/


} // namespace sql
} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)