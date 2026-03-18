#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#include <tobasa/format.h>
#include "tobasasql/exception.h"
#include "tobasasql/adodb_common.h"
#include "tobasasql/adodb_command.h"
#include "tobasasql/adodb_util.h"
#include "tobasasql/adodb_result.h"

namespace tbs {
namespace sql {

AdodbResult::AdodbResult(AdodbConnection* pconn)
   : ResultCommon()
{
   _pResult = nullptr;

   // default options
   _optionCacheData   = false;
   _optionOpenTable   = false;
   _optionCursorType  = ADODB::adOpenStatic;
   _optionCommandType = ADODB::adCmdText;
   _dataCached        = false;
   _pConn             = pconn;
   notifierSource     = "AdodbResult";

   _navigator.init( this );
}

AdodbResult::~AdodbResult()
{
   if (_pResult)
   {
      _pResult.Release();
      _pResult = nullptr;
   }
}

std::string AdodbResult::name() const
{
   return "Adodb Result";
}

void AdodbResult::setOptionOpenTable(bool openTable)
{
   _optionOpenTable = openTable;
   applyOptions();
}

void AdodbResult::setOptionCacheData(bool cache)
{
   _optionCacheData = cache;
   applyOptions();
}

bool AdodbResult::runQuery(const std::string& sql, const AdoParameterCollection& parameters)
{
   if (_pConn == nullptr)
      return false;

   if (_pConn->status() != ConnectionStatus::ok)
      return false;

   applyOptions();

   try
   {
      if (_pConn->logSqlQuery())
         onNotifyDebug(tbsfmt::format("runQuery : {}", sql));

      _qryStr = sql;

      // With Recordset's Open method, we cannot get affected row for action queries(INSER,UPDATE,DELETE)
      // our option is to use Execute method of Command/Connection object, but by doing this we can only
      // have forward-only Recordset, whih means we have to CacheData().

      // TODO_JEFRI : clean this!
      // It is not necessary to test parameters's size, because AdoCommand's createNativeCommand() will check for it internally
      // I just want to use different methods to get Recordset
      if (parameters.size() > 0)
      {
         ADODB::_CommandPtr pCommand = nullptr;
         AdoCommand command(_pConn->nativeConn());

         // Create ADODB::_CommandPtr, and apply parameters
         pCommand = command.createNativeCommand(_qryStr, parameters);

         if (_optionOpenTable)
         {
            // we need to remove this, since _qryStr constains only table name and parameter's size is 0
            // thus program flow will not reach here.

            _pResult.CreateInstance(__uuidof(ADODB::Recordset));

            // Cursor type is adOpenKeyset, CommandType is  adCmdTable

            // Note: https://docs.microsoft.com/en-us/sql/ado/reference/ado-api/open-method-ado-recordset?view=sql-server-ver15
            // If you pass a Command object in the Source argument and also pass an ActiveConnection argument, an error occurs.
            // The ActiveConnection property of the Command object must already be set to a valid Connection object or connection string.
            _pResult->Open(_variant_t((IDispatch*)pCommand, true),
                           vtMissing,
                           _optionCursorType,
                           ADODB::adLockOptimistic,
                           _optionCommandType);
         }
         else
         {
            _variant_t vAffectedRows;
            _pResult = pCommand->Execute(&vAffectedRows, nullptr, _optionCommandType);
            _affectedRows = vAffectedRows.iVal;
         }
      }
      else
      {
         if (_optionOpenTable)
         {
            _pResult.CreateInstance(__uuidof(ADODB::Recordset));

            // Cursor type is adOpenKeyset, CommandType is adCmdTable
            _pResult->Open( util::utf8_to_bstr_t(_qryStr),
                             _variant_t((IDispatch*)_pConn->nativeConn(), true),
                             _optionCursorType,
                             ADODB::adLockOptimistic,
                             _optionCommandType);
         }
         else
         {
            // Note: https://docs.microsoft.com/en-us/sql/ado/reference/ado-api/execute-method-ado-connection?view=sql-server-ver15#remarks
            // The returned Recordset object is always a read-only, forward-only cursor
            // cursor type is default to adOpenForwardOnly,
            _variant_t vAffectedRows;
            _pResult = _pConn->nativeConn()->Execute( 
                          util::utf8_to_bstr_t(_qryStr), &vAffectedRows, _optionCommandType);

            _affectedRows = vAffectedRows.iVal;
         }
      }

      _affectedRows = (int) (_affectedRows < 0) ? 0 : _affectedRows;

      // Note: https://docs.microsoft.com/en-us/office/client-developer/access/desktop-database-reference/open-method-ado-recordset
      // if we perform an action query that doesnt return records, the Recordset returned by such a query will be closed
      if (_pResult->State == ADODB::adStateClosed)
      {
         if (_affectedRows > 0)
         {
            if (_pConn->logExecuteStatus()) 
               onNotifyTrace(tbsfmt::format("SQL command executed successfully, row: {} column: {}, affectedRows: {}", 0, 0, _affectedRows));
            
            _resultStatus = ResultStatus::commandOk;
            return true;
         }
         else
            throw tbs::SqlException("Closed result retrieved from backend.");
      }

      _returnedProperty.retrieve(_pResult);

      // get total columns
      _nColumns = (int) _pResult->Fields->GetCount();

      // we have total columns, now set up columns info
      setupColumnsProperties();

      // Note _pResult is forward-only cursor if _optionCacheData true and optionOpenTable false

      // Note: https://docs.microsoft.com/en-us/sql/ado/reference/ado-api/recordcount-property-ado?view=sql-server-ver15
      // The cursor type of the Recordset object affects whether the number of
      // records can be determined. The RecordCount property will return -1
      // for a forward-only cursor; the actual count for a static or keyset cursor;
      // and either -1 or the actual count for a dynamic cursor, depending on the data source.

      _nRows = _pResult->GetRecordCount();  // Count the correct total records

      // Note : do we really need to do this?
      // according MS document, adOpenStatic and adOpenKeyset return actual rows count.
      if ( (_nRows == -1) && ( _returnedProperty.cursorType == ADODB::adOpenStatic ||
                                 _returnedProperty.cursorType == ADODB::adOpenKeyset )  )
      {
         _nRows = 0;
         if (!_pResult->EndOfFile)
            _pResult->MoveFirst();

         while ( ! _pResult->EndOfFile )
         {
            _nRows++;
            _pResult->MoveNext();
         }

         if (_nRows > 0)
            _pResult->MoveFirst();
      }

      // Total rows for adOpenForwardOnly recordset is -1, we need to cache data to get correct count.
      // Force caching returned data for adOpenForwardOnly recordset
      // or if _optionCacheData set to true
      if ( (_returnedProperty.cursorType == ADODB::adOpenForwardOnly)  || _optionCacheData)
      {
         _nRows = cacheData();
         _dataCached = true;

         if (_nRows > 0)
            onNotifyInfo("Data from backend successfully cached.");

         // all data cached, close Resultset
         if (_pResult && _pResult->State == ADODB::adStateOpen)
         {
            _pResult->Close();
            _pResult = nullptr;
         }
      }
      else
      {
         //auto test = 1;
      }
      
      if (_pConn->logExecuteStatus()) 
         onNotifyTrace(tbsfmt::format("SQL command executed successfully, row: {} column: {}, affectedRows: {}", _nRows, _nColumns, _affectedRows));

      if ( _nRows > 0 )
      {
         // very important to set _resultStatus here.
         // isValid() will read this value.
         _resultStatus = ResultStatus::tuplesOk;
      }
      else
      {
         // very important to set _resultStatus here.
         // isValid() will read this value.
         _resultStatus = ResultStatus::commandOk;
      }

      _navigator.moveFirst();
      return true;
   }
   catch (_com_error& e)
   {
      if (_pResult && _pResult->State == ADODB::adStateOpen)
         _pResult->Close();

      if (_pResult)
      {
         _pResult.Release();
         _pResult = nullptr;
      }

      _resultStatus = ResultStatus::unknown;

      ComError comErr(e/*, __FILE__, __LINE__*/);
      std::string errMsg = tbsfmt::format("Error on runQuery: {}", comErr.fullMessage);
      onNotifyTrace(errMsg);

      throw tbs::SqlException(comErr.description);
   }
   catch (const std::exception& e)
   {
      if (_pResult && _pResult->State == ADODB::adStateOpen)
         _pResult->Close();

      if (_pResult)
      {
         _pResult.Release();
         _pResult = nullptr;
      }

      _resultStatus = ResultStatus::unknown;

      std::string errMsg = tbsfmt::format("Error on runQuery : {}", e.what());
      onNotifyError(errMsg);

      throw tbs::SqlException(e);
   }

   return false;
}

void AdodbResult::connection(AdodbConnection* conn)
{
   _pConn = conn;
}

TypeClass AdodbResult::columnTypeClass(const int columnIndex) const
{
   long colType = columnNativeType(columnIndex);
   return typeClassFromAdodbType(colType);
}

AdodbResult::VariantType AdodbResult::getVariantValue(const int columnIndex) const
{
   long row = _navigator.position();
   throwIfColumnIndexInvalid(columnIndex);
   throwIfRowIndexInvalid(row);

   // auto conversion from _variant_t to VariantType
   _variant_t vValue;
   vValue = getNativeVariant(columnIndex);

   // ADO save SQL Server bigint as DECIMAL _variant_t
   if (_columnInfoCollection[columnIndex].dataType == DataType::bigint)
   {
      if (vValue.vt == VT_NULL)
         return std::monostate{};
      else
      {
         std::string bigintStr = VariantHelper::toString(vValue);
         return std::stoll(bigintStr);
      }
   }

   return VariantHelper::fromNativeVariant(vValue);
}

AdodbResult::VariantType AdodbResult::getVariantValue(const std::string& columnName) const
{
   return getVariantValue(columnNumber(columnName));
}

std::string AdodbResult::getStringValue(const int columnIndex) const
{
   _variant_t vValue;
   vValue = getNativeVariant(columnIndex);

   return VariantHelper::toString(vValue);
}

std::string AdodbResult::getStringValue(const std::string& columnName) const
{
   return getStringValue(columnNumber(columnName));
}

bool AdodbResult::isNullField(const int columnIndex) const
{
   _variant_t vValue;
   vValue = getNativeVariant(columnIndex);

   return vValue.vt == VT_NULL;
}

void AdodbResult::applyOptions()
{
   if (_optionCacheData)
   {
      _optionCursorType = ADODB::adOpenForwardOnly;
      _optionCommandType = ADODB::adCmdText;
   }

   if (_optionOpenTable)
   {
      _optionCursorType = ADODB::adOpenKeyset;
      _optionCommandType = ADODB::adCmdTable;
   }

   if (!_optionCacheData && !_optionOpenTable)
   {
      // back to default
      _optionCursorType = ADODB::adOpenStatic;
      _optionCommandType = ADODB::adCmdText;
   }
}

void AdodbResult::setupColumnsProperties()
{
   if (_nColumns <= 0)
      return;

   _columnInfoCollection.reserve(_nColumns);
   for (int i = 0; i < _nColumns; i++)
   {
      _columnInfoCollection.emplace_back(ColumnInfo());
   }

   try
   {
      _variant_t fieldPos;
      fieldPos.vt = VT_I2;

      ADODB::FieldsPtr pOneRow = _pResult->Fields;

      for (int i = 0; i < _nColumns; i++)
      {
         fieldPos.iVal = i;
         ADODB::FieldPtr pField = pOneRow->GetItem(fieldPos);

         // save column name
         _columnInfoCollection[i].name = util::utf8_from_bstr_t(pField->Name);

         // save column defined size
         _columnInfoCollection[i].definedSize = pField->DefinedSize;

         // save column native type as string
         ADODB::DataTypeEnum fieldType = pField->Type;
         _columnInfoCollection[i].nativeTypeStr = tbs::sql::adoDataTypeToString(fieldType);

         // save column native full type as string
         _columnInfoCollection[i].nativeFullTypeStr = tbs::sql::adoDataTypeToString(fieldType);

         // save column native type
         _columnInfoCollection[i].nativeType = static_cast<long>(fieldType);

         // save column data type : sql::DataType
         _columnInfoCollection[i].dataType = tbs::sql::adoDataTypeToDataType(fieldType);

         // save numeric scale and precision
         _columnInfoCollection[i].numericScale = pField->GetNumericScale();
         _columnInfoCollection[i].precision    = pField->GetPrecision();
      }
   }
   catch (const TypeException & ex)
   {
      onNotifyError(ex.what());
      throw tbs::SqlException(tbsfmt::format("setupColumnProperties, {}", ex.what()), "AodbResult");
   }
}

long AdodbResult::cacheData()
{
   if (_returnedProperty.cursorType != ADODB::adOpenForwardOnly)
   {
      if (!_pResult->EndOfFile)
         _pResult->MoveFirst();
   }

   long totalRows = 0;

   while ( !_pResult->EndOfFile )
   {
      VectorVariant recordVariant;
      if (_nColumns > 0)
         recordVariant.reserve(_nColumns);

      ADODB::FieldsPtr pOneRow = _pResult->Fields;

      int col = 0;
      for (col = 0; col < _nColumns; col++)
      {
         _variant_t vFieldPos;
         vFieldPos.vt = VT_I2;
         vFieldPos.iVal = col;

         ADODB::FieldPtr pField = pOneRow->GetItem(vFieldPos);

         // Note: https://stackoverflow.com/questions/38662438/using-sql-server-datetime2-with-tadoquery-open
         // com_error occured if sql data type is date and DataTypeCompatibility=80  not set in connection string
         const _variant_t vValue = pField->GetValue();

         recordVariant.emplace_back(vValue);
      }

      _dataVariant.emplace_back(recordVariant);
      _pResult->MoveNext();
      totalRows++;
   }

   std::string message = tbsfmt::format("Variant data: rows {}, size is {}", _dataVariant.size(), _dataVariant.capacity());
   onNotifyDebug(message);

   return totalRows;
}

_variant_t AdodbResult::getNativeVariant(const int columnIndex) const
{
   long row = _navigator.position();

   throwIfColumnIndexInvalid(columnIndex);
   throwIfRowIndexInvalid(row);

   _variant_t nativeVariant;

   try
   {
      if (_dataCached)
      {
         VariantType value;
         value = _dataVariant[row][columnIndex];
         nativeVariant = std::get<_variant_t>(value);
      }
      else
      {
         _variant_t vIndex;
         vIndex.vt = VT_I2;
         vIndex.iVal = columnIndex;

         // Note: https://stackoverflow.com/questions/38662438/using-sql-server-datetime2-with-tadoquery-open
         // com_error occured if sql data type is date and DataTypeCompatibility=80  not set in connection string
         nativeVariant = _pResult->GetFields()->GetItem(vIndex)->GetValue();
      }
   }
   catch (_com_error& e)
   {
      ComError comErr(e/*, __FILE__, __LINE__*/);
      std::string errMsg = tbsfmt::format("getNativeVariant, {}", comErr.fullMessage);
      onNotifyError(errMsg);
      throw tbs::SqlException(errMsg, "AdodbResult");
   }
   catch (const SqlException& e)
   {
      throw e;
   }
   catch (const std::exception& e)
   {
      std::string errMsg(tbsfmt::format("getNativeVariant, {}", e.what()));
      throw tbs::SqlException(errMsg, "AdodbResult");
   }

   return nativeVariant;
}

} // namespace sql
} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)