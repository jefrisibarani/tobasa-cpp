#pragma once

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#include <string>
#include <memory>
#include "tobasasql/sql_result_common.h"
#include "tobasasql/adodb_connection.h"
#include "tobasasql/com_variant_helper.h"
#include "tobasasql/adodb_navigator.h"

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief ADO (MSSQL) SQL Result class.
 * \details
 * Backend implementation using `ADODB::_RecordsetPtr` and `RecordVariant`.  
 * - `runQuery()` executes SQL commands, stored procedures, or opens tables/views.  
 * - `setOptionOpenTable(true)` → opens table/view using adOpenKeyset cursor.  
 * - `setOptionCacheData(true)` → uses forward-only cursor (adOpenForwardOnly) and caches all rows into `_dataVariant`.  
 *   After caching, `_pResult` is closed.  
 * - If both options are false, uses adOpenStatic cursor and `_pResult` remains open.
 *
 * Provides full navigation and value retrieval via SqlResult interface.
 */
class AdodbResult : public ResultCommon
{
public:

   using VariantType   = ComVariantType;
   using VariantHelper = ComVariantHelper;
   using VectorVariant = std::vector<ComVariantType>;
   using RecordVariant = std::vector<VectorVariant>;

   AdodbResult(AdodbConnection* pconn = nullptr);
   ~AdodbResult();

   // -------------------------------------------------------
   // Specific implementation methods
   // -------------------------------------------------------

   /// Get implementation class name
   virtual std::string name() const;

   /** 
    * \brief Set operation to open table instead of running sql command.
    * runQuery() option, get data from table or from sql command.
   */
   virtual void setOptionOpenTable(bool openTable = true);

   /**
    * \brief Configure result caching strategy (ADO-specific).
    * \details 
    * This option controls how query results are retrieved and stored, 
    * by adjusting the underlying ADO cursor and command type.
    *
    * ### Behavior
    * | Option combination                | Cursor Type          | Command Type   | Storage                                             |
    * |-----------------------------------|----------------------|----------------|-----------------------------------------------------|
    * | cache = true,  openTable = false  | adOpenForwardOnly    | adCmdText      | Provider cursor (_pResult) — forward-only streaming |
    * | cache = false, openTable = false  | adOpenStatic         | adCmdText      | In-memory copy (_dataVariant) — random access       |
    * | openTable = true (overrides cache)| adOpenKeyset         | adCmdTable     | Direct table recordset (_pResult)                   |
    *
    * - **Forward-only cursor** (cache = true) streams rows directly without materializing.  
    * - **Static cursor** (cache = false) materializes data in memory, supports navigation.  
    * - **Open table** (openTable = true) loads table directly, modifiable if not a view.  
    *
    * \param cache If true, use forward-only cursor (streaming).  
    *              If false, copy results into an in-memory container (RecordVariant)
    */
   virtual void setOptionCacheData(bool cache = true);

   /**
    * \brief Execute a SQL query or open a table using ADO Recordset.(MSSQL via ADO/COM)
    * \details 
    * Behavior depends on run-time options:
    * 
    * - **Default (no options):**
    *   Uses `adOpenStatic` cursor with `adCmdText`.  
    *   Rows remain accessible directly from the live `ADODB::_RecordsetPtr` (`_pResult`).
    *
    * - **setOptionOpenTable(true):**
    *   Treats input as a *table name*, not a SQL query.  
    *   Uses `adOpenKeyset` cursor with `adCmdTable`.  
    *   Works with both parameterized and non-parameterized queries.
    *
    * - **setOptionCacheData(true):**
    *   Forces a forward-only cursor (`adOpenForwardOnly`) and retrieves results 
    *   into the internal cache (`RecordVariant _dataVariant`).  
    *   After caching, the live `_pResult` is closed to free resources, 
    *   and all navigation/reads operate on cached rows.
    *
    * - **Both setOptionOpenTable(true) + setOptionCacheData(true):**
    *   `setOptionOpenTable` takes precedence. Data comes from table access 
    *   (`adOpenKeyset`, `adCmdTable`).
    *
    * Notes:
    * - For action queries (INSERT/UPDATE/DELETE), affected row count is tracked via `_affectedRows`.  
    * - Forward-only cursors normally return `RecordCount = -1`. In that case, results are auto-cached 
    *   into `_dataVariant` to compute row count correctly.
    * - Errors (COM or std::exception) close and release `_pResult` and rethrow as `SqlException`.
    *
    * On success:
    * - `_resultStatus` is set to `ResultStatus::tuplesOk` if rows are returned,
    *   or `ResultStatus::commandOk` for non-row-returning commands.
    *
    * \param sql        SQL command text or table name (depending on options)
    * \param parameters Optional parameter collection
    * \return true on success, false if connection is invalid
    * \throws SqlException on SQL or ADO execution error
    */
   bool runQuery(
      const std::string& sql,
      const AdoParameterCollection& parameters = AdoParameterCollection());

   /// Set specific driver sql connection class implementation
   void connection(AdodbConnection* conn);

   /// Get specific driver sql connection class implementation
   AdodbConnection* connection() const { return _pConn; }

   AdoResultNavigator<AdodbResult>& navigator() { return _navigator; }

   // -------------------------------------------------------
   // Override methods from base class : ResultCommon
   // -------------------------------------------------------

   /** 
    * \brief Get Column Type Class.
    * Column index start at 0.
    * Throws TypeException, SqlException
   */
   virtual TypeClass columnTypeClass(const int columnIndex) const;

   /** 
    * \brief Get Variant value.
    * Get a single field value for specified column name/ position on current row.
    * Column index start at 0.
    * Throws SqlException.
    */
   VariantType getVariantValue(const int columnIndex) const;

   /// Get Variant value.
   VariantType getVariantValue(const std::string& columnName) const;

   /** 
    * \brief Get string value.
    * Get a single field value for specified column name/ position on current row.
    * Column index start at 0.
    * Throws SqlException.
    */
   std::string getStringValue(const int columnIndex) const;

   /// Get string value.
   std::string getStringValue(const std::string& columnName) const;

   /** 
    * \brief Check for null field.
    * Column index start at 0.
    * Throws SqlException.
    */
   virtual bool isNullField(const int columnIndex) const;

private:

   /// Apply Recordset options.
   void applyOptions();

   /** 
    * \brief Setup columns meta data.
    * \details 
    * For SQL Server (tested 2017), sql data type we received from ADO:
    * 
    * bigint         adBigInt       (20)
    * binary         adBinary       (128)
    * bit            adBoolean      (11)
    * char           adChar         (129)
    * date           adVarWChar     (202)
    * datetime       adDBTimeStamp  (135)
    * datetime2      adVarWChar     (202)
    * float          adDouble       (5)
    * int            adInteger      (3)
    * nchar          adWChar        (130)
    * ntext          adLongVarWChar (203)
    * numeric        adNumeric      (131)
    * nvarchar       adVarWChar     (202)
    * real           adSingle       (4)
    * smalldatetime  adDBTimeStamp  (135)
    * text           adLongVarChar  (201)
    * time           adVarWChar     (202)
    * varchar        adVarChar      (200)
    * 
    * To send back sql data type to ADO,
    * \see AdoCommand::createParameter
    */
   void setupColumnsProperties();

   /// Cache retrieved data from database.
   long cacheData();

   /** 
    * \brief Get Native variant (_variant_t) value
    * Get a single field value for specified column name/ position on current row.
    * Column index start at 0.
    * Throws SqlException.
    */
   _variant_t getNativeVariant(const int columnIndex) const;

   /// Sql driver native result object.
   ADODB::_RecordsetPtr _pResult;

   /// Implemented Adodb connection object.
   AdodbConnection* _pConn;

   /// Cached data from backend.
   RecordVariant _dataVariant;

   /** 
    * \brief Recordset cursor type.
    * \details adOpenUnspecified, adOpenForwardOnly, adOpenKeyset, adOpenDynamic, adOpenStatic
    * \see _optionOpenTable \see setOptionOpenTable()
    * \see _optionCacheData \see setOptionCacheData()
    */
   ADODB::CursorTypeEnum _optionCursorType;     // default: ADODB::adOpenStatic;


   /** 
    * \brief Recordset command type.
    * \details adCmdUnspecified, adCmdUnknown, adCmdText, adCmdTable, adCmdStoredProc, adCmdFile, adCmdTableDirect
    * \see _optionOpenTable \see setOptionOpenTable()
    * \see _optionCacheData \see setOptionCacheData()
    */
   ADODB::CommandTypeEnum _optionCommandType;   // default: ADODB::adCmdText;

   struct RecorsetProperty
   {
      void retrieve(ADODB::_RecordsetPtr recordSet)
      {
         cursorType     = recordSet->GetCursorType();
         cursorLocation = recordSet->GetCursorLocation();
         lockType       = recordSet->GetLockType();

         auto command   = (ADODB::_CommandPtr) recordSet->ActiveCommand;
         if (command)
            commandType = command->GetCommandType();
      }

      ADODB::CursorTypeEnum      cursorType;
      ADODB::CursorLocationEnum  cursorLocation;
      ADODB::LockTypeEnum        lockType;
      ADODB::CommandTypeEnum     commandType;
   };

   RecorsetProperty _returnedProperty;

   bool _dataCached;

   friend class AdoResultNavigator<AdodbResult>;
   AdoResultNavigator<AdodbResult> _navigator;
};

} // namespace sql
} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)