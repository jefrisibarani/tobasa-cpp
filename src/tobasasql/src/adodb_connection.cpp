#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#include <tobasa/format.h>
#include "tobasasql/exception.h"
#include "tobasasql/adodb_connection.h"
#include "tobasasql/adodb_util.h"
#include "tobasasql/adodb_command.h"
#include "tobasasql/adodb_result.h"

namespace tbs {
namespace sql {

AdodbConnection::AdodbConnection()
   : ConnectionCommon()
{
   notifierSource = "AdodbConnection";
}

AdodbConnection::~AdodbConnection()
{
   disconnect();
}

std::string AdodbConnection::name() const
{
   return "Adodb Connection";
}

bool AdodbConnection::connect(const std::string& connString)
{
   if (status() == ConnectionStatus::ok)
      return true;

   if (_pConn == nullptr)
      _pConn.CreateInstance(__uuidof(ADODB::Connection));

   if (connString.empty())
   {
      onNotifyDebug(logId() + "Empty connection string not allowed");
      throw tbs::SqlException("Empty connection string not allowed", "adoconn");
   }

   std::string username = "";
   std::string passwd   = "";
   HRESULT     hr       = S_OK;

   try
   {
      _pConn->PutConnectionTimeout(30);

      hr = _pConn->Open( _bstr_t(connString.c_str()),
               "" /*_bstr_t(username.c_str())*/,
               "" /*_bstr_t(passwd.c_str())*/,
               ADODB::adConnectUnspecified);

      if (hr == S_OK)
      {
         _connStatus = ConnectionStatus::ok;
         return true;
      }
   }
   catch (_com_error& e)
   {
      ComError comErr(e/*, __FILE__, __LINE__*/);
      std::string errMsg = tbsfmt::format("Error on connect: {}", comErr.fullMessage);
      onNotifyDebug(logId() + errMsg);

      throw tbs::SqlException(comErr.description, "adoconn");
   }
   catch (const std::exception& e)
   {
      std::string errMsg = tbsfmt::format("Error on connect: {}", e.what());
      onNotifyDebug(logId() + errMsg);

      throw tbs::SqlException(errMsg, "adoconn");
   }

   return false;
}

bool AdodbConnection::disconnect()
{
   if (_pConn != nullptr)
   {
      if (_pConn->GetState() != ADODB::adStateClosed) {
         _pConn->Close();
      }

      _pConn.Release();
      _pConn = nullptr;

      _connStatus = ConnectionStatus::bad;
   }

   return true;
}

ConnectionStatus AdodbConnection::status()
{
   if (_pConn == nullptr)
   {
      _connStatus = ConnectionStatus::bad;
      return _connStatus;
   }

   if ( checkStatus() )
      return _connStatus;
   else 
   {
      _connStatus = ConnectionStatus::bad;
      throw tbs::SqlException("invalid connection object", "AdodbConnection");
   }
}

int AdodbConnection::execute(const std::string& sql, const AdoParameterCollection& parameters)
{
   int affectedRows = 0;
   bool success = execute(sql, affectedRows, parameters);

   if (success)
   {
      if (affectedRows == -1)
         return 0;
      else
         return affectedRows;
   }

   return -1;
}


bool AdodbConnection::execute(const std::string& sql, int& affectedRows,
   const AdoParameterCollection& parameters)
{
   // Note: https://docs.microsoft.com/en-us/sql/ado/reference/ado-api/execute-method-ado-command?view=sql-server-2017
   // ADODB::Connection::Execute's Options parameter: Optional.
   // A Long value that indicates how the provider should evaluate the CommandText
   // or the CommandStream property of the Command object. Can be a bitmask value
   // made using CommandTypeEnum and /or ExecuteOptionEnum values.For example,
   // you could use adCmdTextand adExecuteNoRecords in combination if you want
   // to have ADO evaluate the value of the CommandText property as text, and indicate
   // that the command should discard and not return any records that
   // might be generated when the command text executes.

   if (status() != ConnectionStatus::ok)
      return false;

   try
   {
      if (logSqlQuery())
         onNotifyDebug(logId() + tbsfmt::format("execute : {}", sql));

      _pConn->CursorLocation = ADODB::adUseClient;

      if (parameters.size() > 0)
      {
         AdoCommand command(_pConn);
         bool success = command.execute(sql, affectedRows, parameters);
         if (!success)
            return false;
      }
      else
      {
         _bstr_t sqlCmdBW = util::utf8_to_bstr_t(sql);
         _variant_t vRecords;
         _pConn->Execute( sqlCmdBW, &vRecords,
                     ADODB::adCmdText | ADODB::adExecuteNoRecords);

         affectedRows = vRecords.iVal;
      }

      int rowsRpt = (affectedRows == -1) ? 0 : affectedRows;
      std::string message = tbsfmt::format("SQL command executed successfully, affected rows: {}", rowsRpt);
      
      if (logExecuteStatus()) 
         onNotifyTrace(logId() + message);

      return true;
   }
   catch (_com_error& e)
   {
      ComError comErr(e/*, __FILE__, __LINE__*/);
      std::string errMsg = tbsfmt::format("Error on execute: {}", comErr.fullMessage);
      onNotifyTrace(logId() + errMsg);

      throw tbs::SqlException(comErr.description, "adoconn");
   }
   catch (const std::exception& e)
   {
      std::string errMsg = tbsfmt::format("Error on execute: {}", e.what());
      onNotifyError(logId() + errMsg);

      throw tbs::SqlException(errMsg, "adoconn");
   }

   return false;
}

std::string AdodbConnection::executeScalar(const std::string& sql, const AdoParameterCollection& parameters)
{
   if (status() != ConnectionStatus::ok)
      throw tbs::SqlException("Invalid connection status", "AdodbConnection");

   if (logSqlQuery())
      onNotifyDebug(logId() + tbsfmt::format("executeScalar: {}", sql));

   long nRows = 0;
   ADODB::_RecordsetPtr pRec = nullptr;

   try
   {
      if (parameters.size() > 0)
      {
         AdoCommand command(_pConn);
         pRec = command.executeResult(sql, parameters);
         nRows = pRec->GetRecordCount();
      }
      else
      {
         // No Sql parameters given, execute query using Recordset::Open
         // Here we open Recordset with  ADODB::adOpenStatic, so pRec->GetRecordCount()
         // returns correct value
         HRESULT hr = pRec.CreateInstance(__uuidof(ADODB::Recordset));

         if (hr != S_OK)
            _com_issue_error(hr);

         // Note: copy_conn_to_variant
         // https://docs.microsoft.com/en-us/cpp/cpp/variant-t-variant-t?view=msvc-160
         // _variant_t( IDispatch* pDispSrc , bool fAddRef = true )
         // Constructs a _variant_t object of type VT_DISPATCH from a COM interface pointer. If fAddRef is true,
         // then AddRef is called on the supplied interface pointer to match the call to Release that will occur
         // when the _variant_t object is destroyed. It is up to you to call Release on the supplied interface pointer.
         // If fAddRef is false, this constructor takes ownership of the supplied interface pointer;
         // do not call Release on the supplied interface pointer.

         _bstr_t sqlCmdBW = util::utf8_to_bstr_t(sql);

         pRec->CursorLocation = ADODB::adUseClient;
         pRec->Open(sqlCmdBW,
                  _variant_t((IDispatch*)_pConn, true),  // see note copy_conn_to_variant
                  ADODB::adOpenStatic,
                  ADODB::adLockOptimistic,
                  ADODB::adCmdText);

         nRows = pRec->GetRecordCount();
      }

      std::string result;

      // nRows value may not correct(-1), so recalculate inside while loop
      nRows = 0; // reset nRows
      while (!pRec->EndOfFile)
      {
         if (nRows == 0)
         {
            // We only interested on first record
            ADODB::FieldsPtr pFldLoop = nullptr;
            _variant_t vtIndex;
            vtIndex.vt = VT_I2;           // set vtIndex to save 2 byte int
            pFldLoop = pRec->GetFields(); // get Fields pointer
            vtIndex.iVal = 0;

            _bstr_t result_ = (_bstr_t) pFldLoop->GetItem(vtIndex)->Value;
            result = util::utf8_from_bstr_t(result_);
         }
         nRows++;
         pRec->MoveNext();
      }

      if (nRows < 0)
         onNotifyInfo(logId() + "Could not determine the number of records");
      else if (nRows == 0)
         onNotifyInfo(logId() + "Scalar query returned no row");
      else if (nRows > 1)
         onNotifyInfo(logId() + "Scalar query returned more than one row");

      // Cleaning up COM Object
      if (pRec->State == ADODB::adStateOpen) {
         pRec->Close();
      }

      pRec = nullptr;
      return result;
   }
   catch(_com_error& e)
   {
      pRec = nullptr;

      ComError comErr(e/*, __FILE__, __LINE__*/);
      std::string errMsg = tbsfmt::format("Error on executeScalar: {}", comErr.fullMessage);
      onNotifyTrace(logId() + errMsg);

      throw tbs::SqlException(comErr.description, "adoconn");
   }
   catch(const std::exception& e)
   {
      pRec = nullptr;

      std::string errMsg = tbsfmt::format("Error on executeScalar: {}", e.what());
      onNotifyError(logId() + errMsg);

      throw tbs::SqlException(e, "adoconn");
   }

   return "";
}

std::string AdodbConnection::versionString()
{
   if (status() != ConnectionStatus::ok)
      return "";

   try
   {
      std::string out  = "";
      _bstr_t version  = (_bstr_t) _pConn->Version;
      _bstr_t dbmsName = (_bstr_t) _pConn->Properties->GetItem("DBMS Name")->Value;
      _bstr_t dbmsVer  = (_bstr_t) _pConn->Properties->GetItem("DBMS Version")->Value;
      _bstr_t oledbVer = (_bstr_t) _pConn->Properties->GetItem("OLE DB Version")->Value;
      _bstr_t provName = (_bstr_t) _pConn->Properties->GetItem("Provider Name")->Value;
      _bstr_t provVer  = (_bstr_t) _pConn->Properties->GetItem("Provider Version")->Value;

      _bstr_t fullVersion = dbmsName + " " + dbmsVer + " [ADO " + oledbVer + ", OLEDB "
                           + oledbVer + ", Provider: " + provName + " " + provVer + "]";

      out = std::string( (char*) fullVersion );
      return out;
   }
   catch(_com_error& e)
   {
      ComError comErr(e/*, __FILE__, __LINE__*/);
      std::string errMsg = tbsfmt::format("Error on versionString: {}", comErr.fullMessage);
      onNotifyTrace(logId() + errMsg);
   }
   catch(const std::exception& e)
   {
      std::string errMsg = tbsfmt::format("Error on versionString: {}", e.what());
      onNotifyError(logId() + errMsg);
   }

   return "";
}

std::string AdodbConnection::databaseName()
{
   if (status() != ConnectionStatus::ok)
      return "";
   
   SqlApplyLogInternal applyLogRule(this);
   return executeScalar("SELECT DB_NAME() AS database_name");
}

BackendType AdodbConnection::backendType() const 
{ 
   return BackendType::adodb; 
}

std::string AdodbConnection::dbmsName()
{ 
   return name(); 
}

int64_t AdodbConnection::lastInsertRowid()
{
   if (status() != ConnectionStatus::ok)
      throw tbs::SqlException("Invalid connection status", "MysqlConnection");
         
   SqlApplyLogInternal applyLogRule(this);

   auto rowId = executeScalar("SELECT @@IDENTITY");
   if (rowId.empty())
      throw tbs::SqlException("Invalid last inserted row id", "AdodbConnection");

   if (!util::isNumber(rowId))
      throw tbs::SqlException("Invalid last inserted row id", "AdodbConnection");
      
   return std::stoll( rowId );
}

ADODB::_ConnectionPtr AdodbConnection::nativeConn() const 
{ 
   return _pConn; 
}

bool AdodbConnection::tableOrViewExists(const std::string& tableName, bool checkTable)
{
   // Note: https://docs.microsoft.com/en-us/sql/ado/reference/ado-api/openschema-method-example-vc?view=sql-server-ver15

   bool exists = false;

   if (status() != ConnectionStatus::ok)
      return false;

   ADODB::_RecordsetPtr pRec = nullptr;
   try
   {
      pRec.CreateInstance(__uuidof(ADODB::Recordset));
      pRec->CursorLocation = ADODB::adUseClient;

      std::string tableToCheck(tableName);
      std::string viewOrTableType = (char*)checkTable ? "TABLE" : "VIEW";

      // adSchemaTables has 4 constraint columns :
      // TABLE_CATALOG TABLE_SCHEMA TABLE_NAME TABLE_TYPE
      // we interested in 3rd and 4th column: TABLE_NAME and TABLE_TYPE
      // TABLE_TYPE can be one of SYSTEM TABLE,SYSTEM VIEW, TABLE, VIEW

      // Create a safearray which takes four elements, and pass it as
      // 2nd parameter in OpenSchema method.
      const int nSize = 4;         // Number of elements in the array
      SAFEARRAY* psa = nullptr;    // safe array
      SAFEARRAYBOUND psaBound;     // the bounds of one dimension of the array
      psaBound.lLbound = 0;        // Lower bound for the array, which can be negative
      psaBound.cElements = nSize;  // Number of elements in the array

      psa = ::SafeArrayCreate(VT_VARIANT, 1, &psaBound);
      if (!psa) {
         _com_issue_error(E_OUTOFMEMORY);
      }

      _variant_t  criteria;                  // OpenSchema's criteria
      _variant_t  psaItem[nSize];            // safe array items

      psaItem[2] = tableToCheck.c_str();     // we interested in 3th col: TABLE_NAME
      psaItem[3] = viewOrTableType.c_str();  // we interested in 4th col: TABLE_TYPE

      // Put items to array
      for (long idx = 0; idx < nSize; idx++)
      {
         HRESULT hr = ::SafeArrayPutElement(psa, &idx, &(psaItem[idx]).Detach() );
         if (hr != S_OK)
            _com_issue_error(hr);
      }

      criteria.vt = VT_ARRAY | VT_VARIANT;   // set criteria type to array
      criteria.parray = psa;                 // Initialize criteria with safearray

      pRec = _pConn->OpenSchema(ADODB::adSchemaTables, &criteria);

      if (!(pRec->BOF && pRec->EndOfFile)) {
         exists = true;
      }

      if (pRec && pRec->State == ADODB::adStateOpen) {
         pRec->Close();
      }

      pRec = nullptr;

      /* since criteria is a _variant_t, it will clear/destroy psa automatically
      ::SafeArrayDestroy(psa);
      criteria.vt = VT_EMPTY;
      criteria.parray = nullptr;
      */

      return exists;
   }
   catch (_com_error& e)
   {
      pRec = nullptr;

      ComError comErr(e/*, __FILE__, __LINE__*/);
      std::string errMsg = tbsfmt::format("Error on tableOrViewExists: {}", comErr.fullMessage);
      onNotifyTrace(logId() + errMsg, "adoconn");

      throw tbs::SqlException(comErr.description, "AdodbConnection");
   }
   catch (std::exception& e)
   {
      pRec = nullptr;

      std::string errMsg = tbsfmt::format("Error on tableOrViewExists: {}", e.what());
      onNotifyError(logId() + errMsg, "adoconn");

      throw tbs::SqlException(e, "AdodbConnection");
   }

   return false;
}

bool AdodbConnection::isPrimaryKey(const std::string& columnName, const std::string& tableName)
{
   bool exists = false;

   if (status() != ConnectionStatus::ok)
      return false;

   ADODB::_RecordsetPtr pRec = nullptr;
   try
   {
      pRec.CreateInstance(__uuidof(ADODB::Recordset));
      pRec->CursorLocation = ADODB::adUseClient;

      // adSchemaPrimaryKeys has 3 constraint columns :
      // PK_TABLE_CATALOG PK_TABLE_SCHEMA  PK_TABLE_NAME
      // we interested in 3rd column: PK_TABLE_NAME

      // Create a safearray which takes three elements, and pass it as
      // 2nd parameter in OpenSchema method.
      const int nSize = 3;         // Number of elements in the array
      SAFEARRAY* psa = nullptr;
      SAFEARRAYBOUND psaBound;     // the bounds of one dimension of the array
      psaBound.lLbound = 0;        // Lower bound for the array, which can be negative
      psaBound.cElements = nSize;  // Number of elements in the array

      psa = ::SafeArrayCreate(VT_VARIANT, 1, &psaBound);
      if (!psa) {
         _com_issue_error(E_OUTOFMEMORY);
      }

      _variant_t  criteria;        // OpenSchema's criteria
      _variant_t  psaItem[nSize];  // safe array items

      psaItem[2] = (tableName.c_str()); // we interested in 3th col: PK_TABLE_NAME

      // Put items to array
      for (long idx = 0; idx < nSize; idx++)
      {
         HRESULT hr = ::SafeArrayPutElement(psa, &idx, &psaItem[idx]);
         if (hr != S_OK)
            _com_issue_error(hr);
      }

      criteria.vt = VT_ARRAY | VT_VARIANT;   // set criteria type to array
      criteria.parray = psa;                 // Initialize criteria with safearray

      pRec = _pConn->OpenSchema(ADODB::adSchemaPrimaryKeys, &criteria);

      while (!(pRec->EndOfFile))
      {
         std::string colName((char*)(_bstr_t)pRec->Fields->GetItem("COLUMN_NAME")->Value);
         std::string pkName((char*)(_bstr_t)pRec->Fields->GetItem("PK_NAME")->Value);

         if (columnName == colName)
         {
            exists = true;
            break;
         }

         pRec->MoveNext();
      }

      if (pRec && pRec->State == ADODB::adStateOpen) {
         pRec->Close();
      }

      pRec = nullptr;

      /* since criteria is a _variant_t, it will clear/destroy psa automatically
      ::SafeArrayDestroy(psa);
      criteria.vt = VT_EMPTY;
      criteria.parray = nullptr;
      */

      return exists;
   }
   catch (_com_error& e)
   {
      pRec = nullptr;

      ComError comErr(e/*, __FILE__, __LINE__*/);
      std::string errMsg = tbsfmt::format("Error on isPrimaryKey: {}", comErr.fullMessage);
      onNotifyTrace(logId() + errMsg, "adoconn");

      throw tbs::SqlException(comErr.description, "AdodbConnection");
   }
   catch (std::exception& e)
   {
      pRec = nullptr;

      std::string errMsg = tbsfmt::format("Error on isPrimaryKey: {}", e.what());
      onNotifyError(logId() + errMsg, "adoconn");

      throw tbs::SqlException(e, "AdodbConnection");
   }

   return false;
}

bool AdodbConnection::getTablesOrViews(std::vector<std::string>& objectNames, bool getTables)
{
   if (status() != ConnectionStatus::ok)
      return false;

   ADODB::_RecordsetPtr pRec = nullptr;

   try
   {
      pRec.CreateInstance(__uuidof(ADODB::Recordset));
      pRec->CursorLocation = ADODB::adUseClient;

      std::string viewOrTableType = (char*)getTables ? "TABLE" : "VIEW";

      // adSchemaTables has 4 constraint columns :
      // TABLE_CATALOG TABLE_SCHEMA TABLE_NAME TABLE_TYPE
      // We interested in 4th column: TABLE_TYPE
      // TABLE_TYPE can be one of SYSTEM TABLE, SYSTEM VIEW, TABLE, VIEW

      // Create a safearray which takes four elements,
      // and pass it as 2nd parameter in OpenSchema method.
      const int nSize = 4;         // Number of elements in the array
      SAFEARRAY* psa = nullptr;       // safe array
      SAFEARRAYBOUND psaBound;     // the bounds of one dimension of the array
      psaBound.lLbound = 0;        // Lower bound for the array, which can be negative
      psaBound.cElements = nSize;  // Number of elements in the array

      psa = ::SafeArrayCreate(VT_VARIANT, 1, &psaBound);
      if (!psa) {
         _com_issue_error(E_OUTOFMEMORY);
      }

      _variant_t  criteria;        // OpenSchema's criteria
      _variant_t  psaItem[nSize];      // safe array items

      psaItem[3] = viewOrTableType.c_str(); // we interested in 4th col: TABLE_TYPE

      for (long idx = 0; idx < nSize; idx++)
      {
         HRESULT hr = ::SafeArrayPutElement(psa, &idx, &(psaItem[idx]).Detach());
         if (hr != S_OK)
            _com_issue_error(hr);
      }

      criteria.vt = VT_ARRAY | VT_VARIANT;   // set criteria type to array
      criteria.parray = psa;                 // Initialize criteria with safearray

      pRec = _pConn->OpenSchema(ADODB::adSchemaTables, &criteria);

      while (!(pRec->EndOfFile))
      {
         std::string tableName((char*)(_bstr_t)pRec->Fields->GetItem("TABLE_NAME")->Value);
         objectNames.push_back(tableName);

         pRec->MoveNext();
      }

      if (pRec && pRec->State == ADODB::adStateOpen) {
         pRec->Close();
      }

      pRec = nullptr;

      /* since criteria is a _variant_t, it will clear/destroy psa automatically
      ::SafeArrayDestroy(psa);
      criteria.vt = VT_EMPTY;
      criteria.parray = nullptr;
      */

      return true;
   }
   catch (_com_error& e)
   {
      pRec = nullptr;

      ComError comErr(e/*, __FILE__, __LINE__*/);
      std::string errMsg = tbsfmt::format("Error on getTablesOrViews: {}", comErr.fullMessage);
      onNotifyTrace(logId() + errMsg, "adoconn");

      throw tbs::SqlException(comErr.description, "AdodbConnection");
   }
   catch (std::exception& e)
   {
      pRec = nullptr;

      std::string errMsg = tbsfmt::format("Error on getTablesOrViews: {}", e.what());
      onNotifyError(logId() + errMsg, "adoconn");

      throw tbs::SqlException(e, "AdodbConnection");
   }

   return false;
}

bool AdodbConnection::getPrimaryKeyColumns(std::vector<std::string>& primaryKeyCols, const std::string& tableName)
{
   if (status() != ConnectionStatus::ok)
      return false;

   ADODB::_RecordsetPtr pRec = nullptr;
   try
   {
      pRec.CreateInstance(__uuidof(ADODB::Recordset));
      pRec->CursorLocation = ADODB::adUseClient;

      // adSchemaPrimaryKeys has 3 constraint columns :
      // PK_TABLE_CATALOG PK_TABLE_SCHEMA  PK_TABLE_NAME
      // we interested in 3rd column: PK_TABLE_NAME

      // Create a safearray which takes three elements, and pass it as
      // 2nd parameter in OpenSchema method.
      const int nSize = 3;         // Number of elements in the array
      SAFEARRAY* psa = nullptr;
      SAFEARRAYBOUND psaBound;     // the bounds of one dimension of the array
      psaBound.lLbound = 0;        // Lower bound for the array, which can be negative
      psaBound.cElements = nSize;  // Number of elements in the array

      psa = ::SafeArrayCreate(VT_VARIANT, 1, &psaBound);
      if (!psa) {
         _com_issue_error(E_OUTOFMEMORY);
      }

      _variant_t  criteria;        // OpenSchema's criteria
      _variant_t  psaItem[nSize];  // safe array items

      psaItem[2] = (tableName.c_str()); // we interested in 3th col: PK_TABLE_NAME

      // Put items to array
      for (long idx = 0; idx < nSize; idx++)
      {
         HRESULT hr = ::SafeArrayPutElement(psa, &idx, &psaItem[idx]);
         if (hr != S_OK)
            _com_issue_error(hr);
      }

      criteria.vt = VT_ARRAY | VT_VARIANT;   // set criteria type to array
      criteria.parray = psa;                 // Initialize criteria with safearray

      pRec = _pConn->OpenSchema(ADODB::adSchemaPrimaryKeys, &criteria);

      while (!(pRec->EndOfFile))
      {
         std::string colName((char*)(_bstr_t)pRec->Fields->GetItem("COLUMN_NAME")->Value);
         std::string pkName((char*)(_bstr_t)pRec->Fields->GetItem("PK_NAME")->Value);

         primaryKeyCols.push_back(colName);
         pRec->MoveNext();
      }

      if (pRec && pRec->State == ADODB::adStateOpen) {
         pRec->Close();
      }

      pRec = nullptr;

      /* since criteria is a _variant_t, it will clear/destroy psa automatically
      ::SafeArrayDestroy(psa);
      criteria.vt = VT_EMPTY;
      criteria.parray = nullptr;
      */
      return true;
   }
   catch (_com_error& e)
   {
      pRec = nullptr;

      ComError comErr(e/*, __FILE__, __LINE__*/);
      std::string errMsg = tbsfmt::format("Error on getPrimaryKeyColumns: {}", comErr.fullMessage);
      onNotifyTrace(logId() + errMsg, "adoconn");

      throw tbs::SqlException(comErr.description, "AdodbConnection");
   }
   catch (std::exception& e)
   {
      pRec = nullptr;

      std::string errMsg = tbsfmt::format("Error on getPrimaryKeyColumns: {}", e.what());
      onNotifyError(logId() + errMsg, "adoconn");

      throw tbs::SqlException(e, "AdodbConnection");
   }

   return false;
}


bool AdodbConnection::getAutoIncrementColumns(std::vector<std::string>& autoIncrCols, const std::string& tableName)
{
   // check if column is auto increment
   std::string findAutoIncrementQuery = tbsfmt::format(
      R"-(select column_name from
               (select a.name as table_name , b.name as column_name, b.is_identity
                  from sys.tables a
                  inner join sys.all_columns b on a.object_id = b.object_id
                  where a.type='U' and is_identity=1
                  and a.name = '{}') t; )-", tableName);
   try
   {
      SqlApplyLogInternal applyLogRule(this);

      AdodbResult xresult(this);
      if (!xresult.runQuery(findAutoIncrementQuery))
         return false;

      if (!xresult.isValid())
         return false;

      for (int i = 0; i < xresult.totalRows(); i++)
      {
         xresult.navigator().locate(i);
         autoIncrCols.push_back(xresult.getStringValue(0));
      }

      return true;
   }
   catch (std::exception & e)
   {
      std::string errMsg = tbsfmt::format("Error on getAutoIncrementColumns : {}", e.what());
      onNotifyError(logId() + errMsg, "adoconn");

      throw tbs::SqlException(e, "AdodbConnection");
   }

   return false;
}


} // namespace sql
} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)