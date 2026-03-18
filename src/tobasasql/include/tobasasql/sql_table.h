#pragma once

#include <vector>
#include <cassert>
#include <tobasa/navigator.h>
#include <tobasa/self_counter.h>
#include "tobasasql/sql_connection.h"
#include "tobasasql/column_info.h"
#include "tobasasql/column_lookup_info.h"
#include "tobasasql/column_map.h"
#include "tobasasql/util.h"

namespace tbs {
namespace sql {

// -------------------------------------------------------
// SqlTable was based from pgAdmin III sqlTable class found in file frmEditGrid.h
// -------------------------------------------------------
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $ Id: frmEditGrid.h 6930 2008-01-02 00:10:01Z dpage $
// Copyright (C) 2002 - 2008, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// frmEditGrid.h - The SQL Edit Grid form
//
// -------------------------------------------------------

/**
 * \ingroup SQL
 * \brief Represents a SQL table abstraction.
 * \details 
 *
 * The class automatically caches table data retrieved with 
 * `"SELECT * FROM table_name;"` into its internal cache, allowing modifications 
 * to be made in-memory before persisting them back to the database.  
 *
 * **Restrictions:**  
 * - Modification is only allowed if the underlying object is a real SQL table.  
 * - If the data source is a SQL view, the table will be set as read-only.  
 *
 * **Example usage:**
 * \code
 * // Create a SqlTable
 * sql::SqlTable<SqlDriverType> table("my_table", &sqlConn);
 * table.init();
 *
 * table.moveLast();
 *
 * int curRow = table.getCurrentRowPosition();
 * table.appendRows(2);
 *
 * table.locate(curRow + 1);
 * table.setValue("name", "PT. Angin Ribut HH");
 * table.setValue("address", "Jl. Topan Selatan HH");
 * table.setValue("phone", "081234567890");
 * table.setValue("email", "bos@angin-ribut.com");
 * table.setValue("website", "www.angin-ribut.com");
 * table.saveTable();
 *
 * table.locate(curRow + 2);
 * table.setValue("name", "PT. Angin Tenang KK");
 * table.setValue("address", "Jl. Lautan Teduh KK");
 * table.setValue("phone", "081234564444");
 * table.setValue("email", "bos@angin-tenang.com");
 * table.setValue("website", "www.angin-tenang.com");
 * table.saveTable();
 *
 * // Reload updated data
 * table.reloadData();
 * \endcode
 */
template <typename SqlDriverType>
class SqlTable : public SelfCounter
{
public:
   using ResultImpl      = typename SqlDriverType::ResultImpl;
   using LoggerImpl      = typename SqlDriverType::Logger;
   using SqlResult       = sql::SqlResult<SqlDriverType>;
   using SqlConnection   = sql::SqlConnection<SqlDriverType>;
   using TableHelperImpl = typename SqlDriverType::TableHelperImpl;
   using VariantType     = typename SqlDriverType::VariantType;
   using VectorVariant   = std::vector<VariantType>;
   using VariantHelper   = typename SqlDriverType::VariantHelper;
   using Navigator       = typename SqlDriverType::TableNavigator;
   using Helper          = typename SqlDriverType::HelperImpl;

   /// Alias SqlParameter.
   using SqlParameter = typename SqlDriverType::SqlParameter;
   /// Alias SqlParameterCollection.
   using SqlParameterCollection = typename SqlDriverType::SqlParameterCollection;

protected:
   
   /// Options for retrieving data from backend.
   /// Set options to be used when generating SQL SELECT query
   class TableRetrieveDataOption
   {
   public:

      /// Constructor.
      TableRetrieveDataOption()
      {
         _tableName     = "";
         _pageSize      = 0;
         _pagePosition  = 0;
         _orderBy       = "";
         _dbmsName      = "";
      }

      /// Generate sql select query and return the generated query.
      std::string getSelectQuery()
      {
         std::string selectQuery = "SELECT * FROM " + util::quoteIdent(_tableName);

         if (_parameters.size() > 0)
            selectQuery += " WHERE " + getWhereClause();

         if (!_orderBy.empty())
            selectQuery += " ORDER BY " + _orderBy;

         long offset = (_pagePosition - 1) * _pageSize;
         selectQuery += " " + Helper::limitAndOffset(_pageSize, offset, _dbmsName);
         return selectQuery;
      }

      // Get only WHERE part of the query.
      std::string getWhereClause()
      {
         std::string conditionClause;

         for (size_t i=0; i<_parameters.size(); i++)
         {
            auto param = _parameters.at(i);

            if (!conditionClause.empty())
               conditionClause += " AND ";

            size_t paramPosition = i+1;  // pgsql parameter use 1 based index
            conditionClause += util::quoteIdent(param->name()) + " = :param" + std::to_string(static_cast<int>(paramPosition));
         }

         return conditionClause;
      }

      /// Get SqlParameterCollection.
      SqlParameterCollection& getParameters() { return _parameters; }

      /// Get page size oth te table.
      long getPageSize() const { return _pageSize; }

      /// Set TableHelperImpl to be used.
      //void tableHelper(TableHelperImpl* pHelper) { pHelperImpl = pHelper; }

      /// Set table name, then return reference to this object.
      TableRetrieveDataOption& tableName(const std::string& tableName)
      {
         _tableName = tableName;
         return *this;
      }

      /// Set page size, then return reference to this object.
      TableRetrieveDataOption& pageSize(long pageSize)
      {
         _pageSize = pageSize;
         return *this;
      }

      /// Set page position, then return reference to this object.
      TableRetrieveDataOption& pagePosition(long pagePosition)
      {
         _pagePosition = pagePosition;
         return *this;
      }

      /// Set ORDER BY part of the sql query, then return reference to this object.
      TableRetrieveDataOption& orderBy(const std::string& orderBy)
      {
         _orderBy = orderBy;
         return *this;
      }

      void dbmsName(const std::string& name)
      {
         _dbmsName = name;
      }

   private:

      std::string _tableName;             ///< sql table name
      long        _pageSize;              ///< Total rows in a page/paging (sql's LIMIT)
      long        _pagePosition;          ///< Page Position
      std::string _orderBy;               ///< result order by
      long        _maximumRecordPosition; ///< Maximum row position to browse/get

      std::string _dbmsName;

      /// SqlParameter collection.
      SqlParameterCollection _parameters;
   };

   /** 
    * SqlTable Row.
    */
   class Row
   {
   public:

      /**
       * Constructor.
       * Initilize column vector with Variant type
       */
      Row(int nCols)
         : totalCols(nCols)
         , cached(false)
         , readOnly(false)
         , deleted(false)
         , added(false)
         , changed(false)
      {
         for (int i=0; i<nCols; i++)
         {
            // initialize column vector
            // fill with empty variant(filled with std::monostate)
            columns.insert(columns.end(), VariantType() );
         }
      }

      /// Destructor.
      ~Row() {}

      /// Set column value.
      void setValue(int idx, const VariantType& val)
      {
         // TODO_JEFRI : Fix this, replace with throw
         assert((idx < totalCols) && "Invalid column index");

         // this is a cached row, set this row as changed
         // if value to set is different with cached value
         if (cached)
         {
            if (columns[idx] != val)
               changed = true;
         }

         columns[idx] = val;
      }

      /// Total columns on this row.
      int totalCols;

      /// Cached status. A cached row, contain copied data from backend database.
      bool cached;

      /// Read only status. for example: table without primary key, then the rows set to read only.
      bool readOnly;

      /// Deleted status. Row will be deleted on SqlTable::saveTable().
      bool deleted;

      /// Added status. Row contain values to be inserted on SqlTable::saveTable().
      bool added;

      /// Changed status. If any column has changed, row status changed is true.
      bool changed;

      /// Fields in the row
      VectorVariant columns;
   };

   /** 
    * \brief Retieve data from backend.
    * This function called from inside init()
    * It must initialize _pResult object
    * \return true if success
    * \see init(), _rows, _pResult
    */
   virtual bool retrieveData()
   {
      _logger.debug("[sql] [SqlTable:{}] retrieveData, table {}", selfId(), _tableName);

      _pResult = new SqlResult(_conn);
      if (_pResult == nullptr)
         return false;

      _pResult->setOptionCacheData(false);

      if (!_sqlCommandToRun.empty())
         _pResult->setOptionOpenTable(false);
      else
      {
         // Apply retrieve data options
         if (_retrieveDataOption.getPageSize() > 0)
         {
            // recreate _query
            _query = _retrieveDataOption.getSelectQuery();
            _pResult->setOptionOpenTable(false);
         }
         else
         {
            // _query contains only table name
            // this means we select ALL records
            _pResult->setOptionOpenTable(true);
         }
      }
      // send RetriveDataOption's parameter, which may empty
      bool success = _pResult->runQuery(_query, _retrieveDataOption.getParameters() );

      if (success)
         return true;
      else
      {
         delete _pResult;
         _pResult = nullptr;
         return false;
      }
   }

   /** 
    * \brief Initialize table schema.
    * This function initialize _pColumnsInfo , called from init()
    * \return true if success
    * \see _pColumnsInfo, init(), retrieveData().
    */
   virtual bool initSchema()
   {
      _logger.debug("[sql] [SqlTable:{}] initSchema, table {}", selfId(), _tableName);
      _setReadOnly = _helperImpl.isView( _conn.connImpl());
      bool success = _helperImpl.setupColumnInfo( _pResult->resultImpl(), _pColumnsInfo);

      if (!success)
      {
         _logger.error("[sql] [SqlTable:{}] initSchema failed setting up column information, table {}", selfId(), _tableName);
         return false;
      }

      return true;
   }

   /// Reset rows information values.
   void resetRowGroup()
   {
      _rowsAdded     = 0;
      _rowsCached    = 0;
      _initialRows   = 0;
      _totalRows     = 0;
      _changed       = false;

      for (size_t i=0; i<_rows.size(); i++)
      {
         Row* r = _rows[i];
         if (r != nullptr)
            delete r;
      }

      _rows.clear();
   }

   /// Init rows information values.
   void initRowGroup()
   {
      _rowsAdded  = 0;
      _rowsCached = 0;
      _totalRows  = _initialRows;
      _changed    = false;

      for (long i=0; i<_initialRows; i++)
      {
         Row* r = new Row(_numColumns);
         _rows.push_back(r);
      }
   }

   /** 
    * Get a row for the specified index.
    * If index equals totalRows(), this function creates new empty row
    */
   Row& getRow(long index)
   {
      if (index > _totalRows)
         throw SqlException("Trying to get a row more than total row in RowGroup", "SqlTable");

      if (index < _totalRows)
         return *(_rows.at(index));
      else /* if (index == _totalRows)*/
      {
         Row* row     = new Row(_numColumns);
         row->added   = true;
         row->changed = false;
         row->cached  = false;

         _rows.push_back(row);
         _rowsAdded++;
         _totalRows++;

         return *row;
      }
   }

   /// Delete row.
   void deleteRow(long idx)
   {
      if (idx < _totalRows)
      {
         Row* item = _rows[idx];
         item->cols.clear();
         item->deleted = true;
         _rows.erase(idx);

         _rowsDeleted++;
         _totalRows--;
      }
   }

private:

   /** 
    * \brief Reset table properties and member objects.
    * reloadData() will first call this function before call init()
    * \see reloadData(), init()
    */
   virtual void reset()
   {
      _logger.debug("[sql] [SqlTable:{}] reset state, table {}", selfId(), _tableName);

      resetRowGroup();

      if (_pResult != nullptr)
         delete _pResult;

      _pResult = nullptr;

      if (_pColumnsInfo != nullptr)
         delete[] _pColumnsInfo;

      _pColumnsInfo = nullptr;

      if (_pColumnMaps != nullptr)
         delete[] _pColumnMaps;

      _pColumnMaps = nullptr;

      if (_pSavedRow != nullptr)
         delete _pSavedRow;

      _pSavedRow = nullptr;
   }

   /** 
    * \brief Update changed row to database.
    * This function called from inside saveTable().
    * \see saveTable().
    * \return true in success.
    */
   virtual bool doUpdate()
   {
      bool done = false;

      if (_changed && _lastModifiedRow >= 0)
      {
         int i = 0;
         std::string valList;

         if (_lastModifiedRow >= 0)
         {
            Row& arow = getRow(_lastModifiedRow);

            SqlParameterCollection updateParameters;
            int paramPosition = 0;

            for (i=0; i<_numColumns; i++)
            {
               auto columnInfo = _pColumnsInfo[i];

               // Note:
               // _pSavedRow is a clean copy of a row before changed.
               // Variant value type stored in _pSavedRow, may different from changed row
               // for example, ADODB module, uses _variant_t as internal value of its Variant type
               // when data saved/received in AdoResult, then saved/cached by SqlTable (in init()).
               // When one update a field with setValueXXX() (setValueInt, setValueString),
               // internal value of VariantType is not _variant_t
               // unles expliityly set  with setValue()
               //
               // comparing savedCol with changedCol, may fail, because possibility of different internal data
               //
               auto savedCol           = _pSavedRow->columns[i];
               //auto savedColInternal = std::get<_variant_t>(savedCol);
               auto changedCol         = arow.columns[i];
               //auto changColInternal = std::get<_variant_t>(changCol); // <== This may causing bad_variant_access Exception

               // compare using string version
               std::string strSaved   = VariantHelper::toString(savedCol);
               std::string strChanged = VariantHelper::toString(changedCol);

               if (strSaved != strChanged)
               {
                  paramPosition++;

                  if (!valList.empty()) {
                     valList += " , ";
                  }

                  valList += util::quoteIdent(columnInfo.getName());
                  valList += " = :param" + std::to_string(paramPosition);

                  std::string columnName = columnInfo.getName();
                  sql::DataType dataType = columnInfo.getDataType();
                  long size              = columnInfo.getTypeLength();
                  short decimalDigit     = columnInfo.getNumericScale();

                  if (dataType == DataType::numeric) {
                     size = columnInfo.getPrecision();
                  }

                  VariantType rawValue = arow.columns[i];

                  // Some dbms uses tinyint as boolean (e.g. mysql)
                  // if stored value is cpp's bool, convert back to int (as tinyint)
                  if ( dataType == DataType::tinyint && std::holds_alternative<bool>(rawValue) )
                  {
                     std::string boolVal = VariantHelper::toString(rawValue);
                     rawValue = boolVal=="true" ? 1 : 0;
                  }

                  auto parameter = std::make_shared<SqlParameter>(columnName, dataType, rawValue, size, sql::ParameterDirection::input, decimalDigit);
                  updateParameters.push_back(parameter);
               }
            }

            if (valList.empty()) {
               done = true;
            }
            else
            {
               _logger.debug("[sql] [SqlTable:{}] doUpdate, table {}", selfId(), _tableName);

               std::string whereClause;
               std::string sqlUpdate = "UPDATE " + util::quoteIdent(_tableName)
                                       + " SET " + valList + " WHERE ";

               // pass updateParameters collection since we are going to append more parameter(s)
               whereClause = createPrimaryKeyParameter(_lastModifiedRow, updateParameters);
               if (updateParameters.size() > 0)
               {
                  int changed = _conn.execute(sqlUpdate + whereClause, updateParameters);
                  return (changed >= 0);
               }
               /*
               SqlParameterCollection keyParameters;
               whereClause = createPrimaryKeyParameter(_lastModifiedRow, keyParameters);
               if (keyParameters.size() > 0)
               {
                  // Note: https://stackoverflow.com/a/21972296
                  // move keyParameters content to updateParameters
                  updateParameters.insert(
                     updateParameters.end(),
                     std::make_move_iterator(keyParameters.begin()),
                     std::make_move_iterator(keyParameters.end())
                  );

                  int changed = _conn.execute(sqlUpdate + whereClause, updateParameters);
                  return ( changed >=0 );
               }
               */
            }
         }

         return done;
      }

      return false;
   }

   /** 
    * \brief Insert appended row to database.
    * This function called from inside saveTable().
    * \see saveTable().
    * \return integer value, total inserted rows
    */
   virtual int doInsert()
   {
      if (_lastModifiedRow >= 0)
      {
         Row& arow = getRow(_lastModifiedRow);
         int i = 0;
         std::string colList, valList;

         SqlParameterCollection parameters;
         int paramPosition = 0;

         for (i=0; i<_numColumns; i++)
         {
            auto columnInfo        = _pColumnsInfo[i];
            std::string columnName = columnInfo.getName();
            sql::DataType dataType = columnInfo.getDataType();
            long size              = columnInfo.getTypeLength();
            short decimalDigit     = columnInfo.getNumericScale();

            if (dataType == DataType::numeric)
               size = columnInfo.getPrecision();

            VariantType rawValue = arow.columns[i];
            // Note: Primary key column or auto increment column is a read-only column
            if (!columnInfo.isReadOnly() && VariantHelper::isSet(rawValue))
            {
               paramPosition++;

               if (!colList.empty())
               {
                  valList += ", ";
                  colList += ", ";
               }

               colList += util::quoteIdent(columnName);
               valList += ":param" + std::to_string(paramPosition);

               // Some dbms uses tinyint as boolean (e.g. mysql)
               // if stored value is cpp's bool, convert back to int (as tinyint)
               if ( dataType == DataType::tinyint && std::holds_alternative<bool>(rawValue) )
               {
                  bool boolVal = std::get<bool>(rawValue);
                  rawValue = boolVal ? (int) 1 : (int) 0;
               }

               auto parameter = std::make_shared<SqlParameter>(
                  columnName, dataType, rawValue, size, sql::ParameterDirection::input, decimalDigit);

               parameters.push_back(parameter);
            }
         }

         if (colList.empty())
         {
            std::string msg = "You are trying to insert an empty row to database\n\n";
            msg += tbsfmt::format("Row index in table is {}.\n", _lastModifiedRow);
            _logger.warn("[sql] [SqlTable:{}] doInsert: {}", selfId(), msg);

            return -1;
         }

         _logger.debug("[sql] [SqlTable:{}] doInsert, table {}", selfId(), _tableName);

         std::string sqlquery =
            "INSERT INTO " + util::quoteIdent(_tableName)
            + "(" + colList
            + ") VALUES (" + valList
            + ")";

         int changed = _conn.execute(sqlquery, parameters);
         return changed;
      }
      else
         return 0;
   }

public:

   /** 
    * \brief SqlTable Contructor.
    * \param tableName sql table name in database
    * \param conn SqlConnection object
    * \param sqlcmd sql command query to use instead internal default sqL command query
    * \param isReadOnly if false, modification to table is allowed
    * \note internal sql command query is : SELECT * FROM tableName.
    *        table name in sqlcmd and tableName must equal
    */
   SqlTable(
      SqlConnection&       conn,
      const std::string&   tableName,
      const std::string&   sqlcmd = "",
      bool                 isReadOnly = true)
      : _conn(conn)
      , _tableName {tableName}
      , _pResult {nullptr}
      , _sqlCommandToRun {sqlcmd}
      , _setReadOnly {isReadOnly}
      , _colLabelWithType {false}
      , _valid {false}
      , _pColumnsInfo {nullptr}
      , _pColumnMaps {nullptr}
      , _pSavedRow {nullptr}
      , _query {""}
      , _totalRows {0}
      , _initialRows {0}
      , _numColumns {0}
      , _lastModifiedRow {-1}
      , _lastInsertedRow {-1}
      , _rowsCached {0}
      , _rowsAdded {0}
      , _rowsDeleted {0}
      , _lastSuccessfullyDeletedRows {0}
   {
      _helperImpl.setTableName(_tableName);
      _optionPageSize = 10;
      _pageCount = 1;

      _retrieveDataOption.tableName(_tableName);
      _retrieveDataOption.dbmsName(_conn.dbmsName());

      // init table row navigator, passing totalRows()
      _navigator.init( std::bind(&SqlTable::totalRows, this) );
   }

   /// SqlTable Destructor.
   virtual ~SqlTable()
   {
      for (size_t i=0; i<_rows.size(); i++)
      {
         Row* r = _rows[i];
         delete r;
      }

      _rows.clear();

      if (_pResult) {
         delete _pResult;
      }

      delete[] _pColumnsInfo;
      delete[] _pColumnMaps;

      if (_pSavedRow)
         delete _pSavedRow;
   }

   /** 
    * \brief Reload data from backend.
    * \details This function does not recreate column metadata. It only reloads table content from database.
    * If sqlcmd empty this function do nothing.
    * \param sqlcmd sql command query to use instead internal default sql command query
    * \param isReadOnly if false, modification to table is allowed
    * \note internal sql command query is : SELECT * FROM tableName.
    *       table name in sqlcmd and tableName must equal
    */
   bool reloadData(const std::string& sqlcmd, bool isReadOnly)
   {
      if (sqlcmd.empty())
         return false;

      _sqlCommandToRun = sqlcmd;
      _setReadOnly = isReadOnly;

      return reloadData();
   }

   /** 
    * \brief Reload data from backend.
    * \details Reload data from database, with stored SQL command.
    * This function does not recreate column metadata. It only reloads table content from database.
    */
   bool reloadData()
   {
      _logger.debug("[sql] [SqlTable:{}] reloadData, table {}", selfId(), _tableName);

      reset();

      _valid                        = false;
      _totalRows                    = 0;
      _initialRows                  = 0;
      _numColumns                   = 0;
      _lastModifiedRow              = -1;
      _rowsCached                   = 0;
      _rowsAdded                    = 0;
      _rowsCached                   = 0;
      _rowsDeleted                  = 0;
      _lastSuccessfullyDeletedRows  = 0;

      _navigator.moveFirst();

      init();

      return true;
   }

   /** 
    * \brief Initialize table content.
    * \details Initialize table schema and get table content from database
    * then cache database content to internal buffer
    * \see reloadData(), initSchema(), _rows, _pResult
    */
   virtual void init()
   {
      _logger.debug("[sql] [SqlTable:{}] init, table {}", selfId(), _tableName);

      if (!_sqlCommandToRun.empty())
         _query = _sqlCommandToRun;
      else
         _query = _tableName;


      if ( _conn.status() != ConnectionStatus::ok )
      {
         _valid = false;
         return;
      }

      bool ok = false;
      ok = retrieveData();

      if (ok)
      {
         _initialRows   = _pResult->totalRows();
         _numColumns    = _pResult->totalColumns();
         _pColumnsInfo  = new ColumnInfo[_numColumns];
         _pColumnMaps   = new ColumnMap<SqlResult>[_numColumns];
         _pSavedRow     = new Row(_numColumns);

         if (_initialRows > 0)
            initRowGroup();

         if (initSchema())
         {
            if (!_setReadOnly)
            {
               // default is let user add row manualy.
               // an empty line waiting for inserts
               //_rowsAdded=1;
            }

            _valid = true;
            _navigator.moveFirst();

            _logger.debug("[sql] [SqlTable:{}] init, table: {}  initialized with {} rows and {} columns",
               selfId(), _tableName, totalRows(), totalColumns());
         }
         else
            _valid = false;
      }
      else
      {
         _valid = false;
         _logger.warn("[sql] [SqlTable:{}] init, table: {}  retrieveData failed", selfId(), _tableName);
      }

      if (!_valid) {
         return;
      }

      // at this point _totalRows equals _initialRows
      for ( int i=0; i<_initialRows; i++ )
      {
         _pResult->locate(i);

         // i must < _initialRows, so getRow() will not create new row.
         Row& arow = getRow(i);

         for ( int j=0; j<_numColumns; j++ )
         {
            VariantType valVariant;
            valVariant = _pResult->getVariantValue(j);
            arow.setValue(j, valVariant);

            if (j == _numColumns - 1)
               arow.cached = true;
         }

         _rowsCached++;
      }

      // Delete result set if all rows already cached
      if (_rowsCached == _initialRows)
      {
         delete _pResult;
         _pResult = nullptr;
      }

      // setup paging
      if (_initialRows >= _optionPageSize)
      {
         int remainder = _initialRows % _optionPageSize;
         _pageCount = _initialRows / _optionPageSize;

         if (remainder > 0)
            _pageCount += 1;
      }
   }

   /** 
    * Return table status.
    * Table is valid if retrieveData() & initSchema() returns true.
    * \return true if table is valid.
    */
   inline bool isValid() { return _valid;}

   /** 
    * \brief Save table data to database.
    * \details This function will save canged/added table content to database.
    * Depend of the changes, it may call doUpdate() or doInsert()
    * \return true in success.
    */
   virtual bool saveTable()
   {
      bool done = true;

      if (_lastModifiedRow >= 0)
      {
         Row& arow = getRow(_lastModifiedRow);
         if (arow.changed)
         {
            // UPDATE
            done = doUpdate();
            // if update ran successully, then set row unchanged.
            //if (done)
            //  arow.changed = false;
         }
         else if (arow.added)
         {
            // INSERT
            int result = doInsert();
            if (result > 0)
            {
               _logger.debug("[sql] [SqlTable:{}] saveTable, {} row(s) inserted to table {}", selfId(), result, _tableName);

               done = true;
               _lastInsertedRow = _lastModifiedRow;
               _rowsCached++;

               // Read back what we inserted to get default values
               SqlParameterCollection parameters;
               std::string primaryKeyClause = createPrimaryKeyParameter(_lastModifiedRow, parameters);

               if ( parameters.size() > 0 )
               {
                  std::string selectCommand;
                  selectCommand = "SELECT * FROM " + util::quoteIdent(_tableName) + " WHERE ";

                  //SqlApplyLogInternal applyLogRule(&_conn); // disable log temporarily

                  SqlResult tmpResult(_conn);
                  tmpResult.runQuery(selectCommand + primaryKeyClause, parameters);
                  if (tmpResult.isValid() && tmpResult.totalRows()>0)
                  {
                     for (int i=0; i<_numColumns; i++)
                     {
                        arow.columns[i] = tmpResult.getVariantValue(_pColumnsInfo[i].getName());
                     }
                  }
               }
               else
               {
                  // That's a problem: obviously, the key generated isn't present
                  // because it's serial or default or otherwise generated in the backend we don't get.
                  // That's why the whole line is declared readonly.
                  _logger.warn("[sql] [SqlTable:{}] saveTable, could not read generated key from database {}", selfId(), _tableName);
                  arow.readOnly = true;
               }
            }
            else {
               done = false;
            }
         }

         if (done)
         {
            arow.added = false;
            arow.cached = true;
            _lastModifiedRow = -1;
         }
      }

      return done;
   }

   // -------------------------------------------------------
   // Navigations
   // -------------------------------------------------------

   /// Move to the next row.
   void moveNext() { _navigator.moveNext(); }

   /// Move to the previous row.
   void movePrevious() { _navigator.movePrevious(); }

   /// Move to the first row.
   void moveFirst() { _navigator.moveFirst(); }

   /// Move to the last row.
   void moveLast() { _navigator.moveLast(); }

   /// Set current row, zero based index.
   void locate(long newRow) { _navigator.locate(newRow); }

   bool isBof() const { return _navigator.isBof(); }
   bool isEof() const { return _navigator.isEof(); }

   /// locate page, zero based index.
   void locatePage(long pagePosition)
   {
      if (pagePosition <= _pageCount)
      {
         long rowPosition = (pagePosition-1) * _optionPageSize;
         locate(rowPosition);
      }
   }

   /// Get ccurrent row position.
   long currentRowPosition() const { return _navigator.position(); }

   /// Append row(s) to table.
   virtual bool appendRows(size_t rows)
   {
      unsigned int i = 0;
      for (i=0; i<rows; i++)
      {
         // by calling getRow(_totalRows), getRow() will append new rows
         // and inside getRow(),  will be increased by one
         // everytime we call getRow(_totalRows)
         getRow(_totalRows);
      }

      return true;
   }

   /// Delete row from table starting from Pos as much Rows.
   virtual bool deleteRows(int pos, int rows)
   {
      int i = pos;
      int rowsDone = 0;

      while (i < pos + rows)
      {
         Row& arow = getRow(i);

         if (arow.cached)
         {
            SqlParameterCollection parameters;
            std::string primaryKeyClause = createPrimaryKeyParameter(i, parameters);
            std::string sqlDelete = "DELETE FROM " + util::quoteIdent(_tableName) + " WHERE ";

            if (parameters.size() > 0)
            {
               bool done = _conn.executeVoid(sqlDelete + primaryKeyClause, parameters);
               if (done)
               {
                  Row* item = _rows[i];
                  delete item;

                  _rows.erase(_rows.begin() + i);

                  _rowsDeleted++;
                  _totalRows--;
                  _rowsCached--;

                  rowsDone++;
               }
               else {
                  break;
               }
            }
         }

         i++;
      }

      _lastSuccessfullyDeletedRows = (int) rowsDone;
      return (rowsDone != 0);
   }

   /// Check if table is empty.
   /// Table considered empty if totalRows() equals 0
   bool isEmpty() const { return _totalRows == 0; }

   /// Check if a cell is empty.
   bool isEmptyCell(int row, int col) { return false;}

   /// Check if a column needs resizing (what is this?).
   bool needsResizing(int col) { return _pColumnsInfo[col].isNeedResize() ;}

   /// Is column in current row is a text column?.
   bool isColText(int col)
   {
      if (_valid)
         return !_pColumnsInfo[col].isNumeric();
      else
         return true;
   }

   /// Get total column count.
   int totalColumns() const { return _numColumns; }

   /// Get total actual rows count, which is not always same as initialRowsCount().
   long totalRows() const { return _totalRows; }

   /// Get last successfully deleted row.
   int lastSuccessfullyDeletedRows() const { return _lastSuccessfullyDeletedRows;}

   /** 
    * Get initital rows count.
    * After appendRow() or deleteRow(), initial rows count changed. Use totalRows() to get last actual rows count
    */
   int initialRowsCount() const { return _initialRows; }

   /** 
    * Get last modified row in table.
    * Last modified row is a last row which last affected by setValue()
    * or set by setRowToEdit().
    * \return an integer >= 0 if there is a modified row inside internal cache /_rows.
    * \see setValue(), setRowToEdit().
    */
   int lastModifiedRow() { return _lastModifiedRow; }

   /// Get last inserted row in table, after a succesfull INSERT to backend.
   int lastInsertedRow() { return _lastInsertedRow;}

   /// Get total deleted rows.
   int rowsDeleted() { return _rowsDeleted;}

   /// Get total added/appended row.
   int rowsAdded() { return _rowsAdded;}

   /// Get total cached rows.
   int rowsCached() { return _rowsCached;}

   /** 
    * \brief Set a row to be edited.
    * \details Set a row to be edited (this applied to both INSERT/UPDATE).
    * If we are going to Insert,then before calling this function,
    * we must call appendRow(1) to append an empty blank row in the table internal cache/_rows
    * thus incresing _totalRows
    * \param row index.
    * \see lastModifiedRow(), appendRow().
    */
   void setRowToEdit(long row) { _lastModifiedRow=row;}

   /// Get table name.
   inline std::string tableName() const { return _tableName;}

   virtual VariantType getVariantValue(const int row, const int col)
   {
      VariantType value;

      if (row > _totalRows || row < 0 || col >= _numColumns || col < 0)
      {
         _logger.error("[sql] [SqlTable:{}] getVariantValue, invalid field index [row={},col={}] for table {}", row, col, selfId(), _tableName);
         return value;
      }

      // Note, if row equals _totalRows, getRow() will ceate new empty row.
      Row& arow = getRow(row);
      value = arow.columns[col];
      return value;
   }

   virtual VariantType getVariantValue(const std::string& columnName)
   {
      return getVariantValue(getColumnPosition(columnName));
   }

   virtual VariantType getVariantValue(int col)
   {
      return getVariantValue(_navigator.position(), col);
   }

   /** 
    * \brief Get value from specified row and column.
    * \details Note, with empty table and currentRowPosition() == 0,  and you call getStringValue("column_name"),
    * totalRows() will return 1. When working with Form, do not call getStringValue() from inside
    * Form::doTransferDataToControl() or Form::setFormEditable() \n
    * Always call getStringValue() if table is not empty
    * \code
    *    if(!_pTable->isEmpty())
    *    {
    *       _code=_pTable->getStringValue("code");
    *    }
    * \endcode
    */
   virtual std::string getStringValue(int row, int col)
   {
      std::string val;

      // in case invalid index, just return empty string
      if (row > _totalRows || row < 0 || col >= _numColumns || col < 0)
      {
         _logger.error("[sql] [SqlTable:{}] getStringValue, invalid field index [row={},col={}] for table {}", row, col, selfId(), _tableName);
         return val;
      }

      // Note, if row equals _totalRows, getRow() will ceate new empty row,
      // with fields contains variant value initialized with std::monostate
      Row& arow = getRow(row);

      if (_pColumnsInfo[col].getTypeClass() == TypeClass::blob)
         return sql::BLOBSTR;

      auto variantValue = arow.columns[col];

      if (!VariantHelper::isSet(variantValue) /*|| VariantHelper::isEmpty(variantValue)*/)
         return sql::NULLSTR;

      val = VariantHelper::toString(variantValue);

      return val;
   }

   /// Get string value.
   std::string getStringValue(int col)
   {
      return getStringValue(_navigator.position(), col);
   }

   /// Get string value.
   std::string getStringValue(const std::string &colname)
   {
      return getStringValue(getColumnPosition(colname));
   }

   /// Get column position/index.
   int getColumnPosition(const std::string &columnName)
   {
      int colNumber = -1;

      for (int i=0; i<_numColumns; i++)
      {
         if (_pColumnsInfo[i].getName() == columnName )
         {
            colNumber = i;
            break;
         }
      }

      if (colNumber == -1) {
         _logger.error("[sql] [SqlTable:{}] getColumnPosition: table {}, name:{} error : non existent column: {}", selfId(), _tableName, columnName, columnName);
      }

      return colNumber;
   }

   /** 
    * \brief Find row position of value located in column.
    * column is zero base index  and is a column which either primary key column or
    * column with unique values
    */
   int getRowPosition(const VariantType& value, int column)
   {
      int ret = 0;
      int i = 0;

      for (i=0; i<totalRows(); i++)
      {
         VariantType realVal = getVariantValue(i, column);
         if (realVal == value) {
            break;
         }
      }

      ret = i;
      return ret;
   }

   /// Set value with VariantType.
   void setValue(int row, int col, const VariantType& value)
   {
      if (row>=0 && row < _totalRows && col>=0 && col < _numColumns)
      {
         auto columnInfo = _pColumnsInfo[col];
         // Note: auto increment column is a read-only column
         if (columnInfo.isReadOnly())
            throw tbs::SqlException("set value on read only column is not allowed", "SqlTable");

         /*
         // TODO_JEFRI : allow setValue() with empty value?
         if (VariantHelper::isEmpty(value))
         {
            _logger.warn("[sql] [SqlTable:{}] setValue, setting empty value is forbidden, table:{}, row:{}, col:{}",
                  selfId(), _tableName, row, col);
            return;
         }
         */

         // get Row object referene pointed at row
         Row& arow = getRow(row);

         // if the row refrerence we got is a cached row, copy all column values to _pSavedRow
         // so we have a clean copy of the row
         // later on doUpdate(), we will use it to compare changing column values
         if (arow.cached && !arow.changed)
         {
            int i = 0;
            for (i=0; i<(int)_numColumns; i++)
            {
               _pSavedRow->setValue(i, arow.columns[i]);
            }
         }

         arow.setValue(col, value);
         // if we are changing values in a cached row, row state's "changed" will be set to true
         // this mean we will UPDATE the table
         if (arow.changed)
         {
            _changed = true;
            _lastModifiedRow = row;
         }

         if (arow.added)
            _lastModifiedRow = row;
      }
      else
         throw tbs::SqlException("invalid row or column index in setValue", "SqlTable");
   }

   /// Set value with VariantType.
   void setValue(int col, const VariantType& value)
   {
      setValue(_navigator.position(), col, value);
   }

   /// Set value with VariantType.
   void setValue(const std::string& colname, const VariantType& value)
   {
      setValue(_navigator.position(), getColumnPosition(colname), value);
   }

   /// Set value with std::string type.
   void setValueString(int col, const std::string& value)
   {
      setValue( col, VariantType(value));
   }
   /// Set value with std::string type.
   void setValueString(const std::string& colname, const std::string& value)
   {
      setValue(colname, VariantType(value));
   }

   /*************** SQL Small Integer ***************/

   /// Set value with int type (short/sql small integer).
   void setValueSmallInt(int col, int value)
   {
      setValue(col, VariantType(value));
   }
   /// Set value with int type (short/sql small integer).
   void setValueSmallInt(const std::string& colname, int value)
   {
      setValue(colname, VariantType(value));
   }

   /// Set value with int type (short/sql small integer).
   void setValueInt(int col, int value)
   {
      setValue(col, VariantType(value));
   }
   /// Set value with int type (short/sql small integer).
   void setValueInt(const std::string& colname, int value)
   {
      setValue(colname, VariantType(value));
   }

   /*************** SQL Integer ***************/

   /// Set value with sql integer.
   void setValueInteger(int col, long value)
   {
      setValue(col, VariantType(value));
   }
   /// Set value with sql integer.
   void setValueInteger(const std::string& colname, long value)
   {
      setValue(colname, VariantType(value));
   }


   /// Set value with long type (sql integer).
   void setValueLong(int col, long value)
   {
      setValue(col, VariantType(value));
   }
   /// Set value with long type (sql integer).
   void setValueLong(const std::string& colname, long value)
   {
      setValue(colname, VariantType(value));
   }

   /*************** SQL Big integer ***************/

   /// Set value with sql big integer.
   void setValueBigInteger(int col, long long value)
   {
      setValue(col, VariantType( static_cast<int64_t>(value) ));
   }
   /// Set value with sql big integer.
   void setValueBigInteger(const std::string& colname, long long value)
   {
      setValue(colname, VariantType( static_cast<int64_t>(value) ));
   }


   /*************** Float/Real/Float4 ***************/

   /// Set value with float type.
   void setValueFloat(int col, float value)
   {
      setValue(col, VariantType(value));
   }
   /// Set value with float type.
   void setValueFloat(const std::string& colname, float value)
   {
      setValue(colname, VariantType(value));
   }

   /*************** Double/Float8 ***************/

   /// Set value with double type.
   void setValueDouble(int col, double value)
   {
      setValue(col, VariantType(value));
   }
   /// Set value with double type.
   void setValueDouble(const std::string& colname, double value)
   {
      setValue(colname, VariantType(value));
   }

   /*************** Boolean ***************/

   /// Set value with bool type.
   void setValueBool(int col, bool value)
   {
      setValue(col, VariantType(value));
   }

   /// Set value with bool type.
   void setValueBool(const std::string& colname, bool value)
   {
      setValue(colname, VariantType(value));
   }

   /// Set table name.
   void setTableName(const std::string &tableName) { _tableName = tableName; }

   /// Is a row read only.
   /// For non exist row (row > totalRows(), this funcction return false)
   virtual bool isRowReadOnly(int row)
   {
      if (row < _totalRows)
         return getRow(row).readOnly;
      else
         return false;
   }

   /// Is a row cached.
   virtual bool isRowCached(int row)
   {
      return getRow(row).cached;
   }

   /// Get column label value.
   virtual std::string getColLabelValue(int col) const
   {
      std::string label;

      if (!_valid)
         return label;

      if (_pColumnsInfo)
      {
         ColumnInfo& info = _pColumnsInfo[col];

         if (info.hasAlias() && info.useAlias())
            label = info.getAlias();
         else
            label = info.getName();

         if (_colLabelWithType)
         {
            label += "\n";
            label += info.getDisplayTypeName();
         }
      }

      return label;
   }

   /// Get column label value.
   virtual std::string getRowLabelValue(int row)
   {
      std::string label;

      if (isRowCached(row))
         label = tbsfmt::format("{}", row + 1);
      else
         label = "*";

      return label;
   }

   /** 
    * Set table to also display column data type along with column name.
    * \param show if true, column type will be added to column label
   */
   void setColumnLabelWithType(bool show=true) { _colLabelWithType = show;}

   /// Get ColumnInfo array pointer for all columns.
   inline ColumnInfo* columnInfo() { return _pColumnsInfo;}

   /// Get ColumnInfo reference for a single column.
   inline ColumnInfo& getColumnInfo(int col) { return _pColumnsInfo[col];}

   /// Set Number editor.
   void setNumberEditor(int col, int len) { _pColumnsInfo[col].setNumeric(); }

   /// Is table read only.
   inline bool isReadOnly() { return _setReadOnly; }

   /// Set table read only.
   inline void setReadOnly(bool val=true) { _setReadOnly = val;}

   /** 
    * \brief Create/Setup column lookup.
    * \details Sometime, a column need to lookup some data from other source/table
    * the following functions provide easy way to setup a lookup for specific column or lots of columns at once.
    * these function must be called after contructor
    * This is useful when we want to display/set column value within/from combobox
    * \param referencingColumn a column in table which referencing a column in other table.
    * \param referencedTable.
    * \param orderBy ORDER BY clause eg: "column_id ASC".
    * \param needMapping If true then a mapping created.
    * \see ColumnLookupInfo.
    */
   bool setupColumnLookups(const std::vector<ColumnLookupInfo*> &colLookups)
   {
      if (!colLookups.empty())
      {
         for (std::vector<ColumnLookupInfo*>::const_iterator it = colLookups.begin(); it != colLookups.end(); ++it)
         {
            ColumnLookupInfo* lookup = *it;

            int colIndex = -1;

            if (lookup->getColIndex() == -1)
               colIndex = getColumnPosition(lookup->getReferencingColumn());
            else
               colIndex = lookup->getColIndex();

            if (colIndex > _numColumns)
               return false;

            if (colIndex < 0) // we have column lookup for non-existent column
               return false;

            std::string sql = lookup->getSql();
            bool needMapping = lookup->needMapping();

            SqlResult dataSet(_conn);
            dataSet.runQuery(sql);
            if (dataSet.isValid() && dataSet.totalRows() > 0)
            {
               _pColumnMaps[colIndex].init(dataSet, needMapping);
               _pColumnsInfo[colIndex].setHasDataLookup();
            }
            else
               return false;
         }
         return true;
      }

      return false;
   }

   /// Setup column lookup.
   bool setupColumnLookups(
      const std::string &referencingColumn,
      const std::string &referencedTable,
      const std::string& orderBy,
      bool needMapping)
   {
      int idx = getColumnPosition(referencingColumn);
      return setupColumnLookups(idx, referencedTable, orderBy, needMapping);
   }

   /// Setup column lookup.
   bool setupColumnLookups(
      int col,
      const std::string &referencedTable,
      const std::string& orderBy,
      bool needMapping)
   {
      if (col > _numColumns)
         return false;

      if (col < 0)
         return false;

      std::string sql;
      sql = "SELECT * FROM " + util::quoteIdent(referencedTable);

      if (!orderBy.empty())
         sql += " ORDER BY " + orderBy;

      SqlResult dataSet(_conn);
      dataSet.runQuery(sql);
      if (dataSet.isValid() && dataSet.totalRows() > 0)
      {
         _pColumnMaps[col].init(dataSet, needMapping);
         _pColumnsInfo[col].setHasDataLookup();

         return true;
      }

      return false;
   }

   /// Setup column lookup.
   bool setupColumnLookups(
      const std::string &referencingColumn,
      const std::string &sql,
      bool needMapping)
   {
      int idx = getColumnPosition(referencingColumn);
      return setupColumnLookups(idx, sql, needMapping);
   }

   /// Setup column lookup.
   bool setupColumnLookups(
      int col,
      const std::string &sql,
      bool needMapping)
   {
      if (col > _numColumns)
         return false;

      if (col < 0)
         return false;

      SqlResult dataSet(_conn);
      dataSet.runQuery(sql);
      if (dataSet.isValid() && dataSet.totalRows() > 0)
      {
         _pColumnMaps[col].init(dataSet, needMapping);
         _pColumnsInfo[col].setHasDataLookup();

         return true;
      }

      return false;
   }

   /// Setup column lookup.
   bool setupColumnLookups(
      const std::string &referencingColumn,
      const std::string &refdColKey,
      const std::string &refdColVal,
      const std::string &referencedTable,
      const std::string &orderBy,
      bool needMapping=true)
   {
      return setupColumnLookups(referencingColumn, refdColKey, refdColVal, referencedTable, orderBy, "", needMapping);
   }

   /// Setup column lookup.
   bool setupColumnLookups(
      const std::string &referencingColumn,
      const std::string &refdColKey,
      const std::string &refdColVal,
      const std::string &referencedTable,
      const std::string &orderBy,
      const std::string &whereClause,
      bool needMapping=true)
   {
      std::string sql;

      sql += "SELECT " + util::quoteIdent(refdColKey) + ", ";
      sql += util::quoteIdent(refdColVal) + " FROM " + util::quoteIdent(referencedTable);

      if (!whereClause.empty())
         sql += " WHERE " + whereClause;

      if (!orderBy.empty())
         sql += " ORDER BY " + orderBy;

      return setupColumnLookups(referencingColumn, sql, needMapping);
   }

   /// Setup column lookup.
   ColumnMap<SqlResult>* getColumnLookups(const std::string &colname)
   {
      int idx = getColumnPosition(colname);
      return getColumnLookups(idx);
   }

   /// Setup column lookup.
   ColumnMap<SqlResult>* getColumnLookups(int col)
   {
      if (col > _numColumns || col < 0)
         return nullptr;

      if (_pColumnsInfo[col].hasDataLookup())
         return _pColumnMaps[col];

      return nullptr;
   }


   /// Get sql query command.
   std::string sqlQuery() const { return _query;}

   /** 
    * \brief Create WHERE clause from table Primary Key(s).
    * example: Ado   : id=?  AND name=?
    *          Pgsql : id=$1 AND name=$2
    */
   virtual std::string createPrimaryKeyParameter(int row, SqlParameterCollection& parameters)
   {
      Row* pRow = nullptr;
      Row& _row = getRow(row);

      if (_row.changed)
         pRow = _pSavedRow;
      else
         pRow = &_row;

      std::string conditionClause;

      for (int i=0; i<_numColumns; i++)
      {
         auto columnInfo = _pColumnsInfo[i];

         if (columnInfo.isPrimaryKey())
         {
            VariantType rawValue = pRow->columns[i];
            std::string columnName = columnInfo.getName();

            if ( !VariantHelper::isSet(rawValue) || VariantHelper::isEmpty(rawValue) )
            {
               _logger.warn("[sql] [SqlTable:{}] createPrimaryKeyParameter, primary key column {} is empty", selfId(), columnName);
               continue;
            }

            sql::DataType dataType = columnInfo.getDataType();
            long size = columnInfo.getTypeLength();

            if (!conditionClause.empty())
               conditionClause += " AND ";

            // set param position, calculating from parameters collection size
            size_t paramPosition = parameters.size();
            conditionClause += util::quoteIdent(columnName) + " = :param" + std::to_string(static_cast<int>(paramPosition)+1);

            auto parameter = std::make_shared<SqlParameter>(columnName, dataType, rawValue, size, sql::ParameterDirection::input);
            parameters.push_back(parameter);
         }
      }

      return conditionClause;
   }

   void setOptionPageSize(long pageSize)
   {
      _optionPageSize = pageSize;
   }

   TableRetrieveDataOption& getRetrieveDataOption()
   {
      return _retrieveDataOption;
   }

protected:

   SqlConnection&        _conn;         ///< SqlConnection object we are running on
   SqlResult*            _pResult;      ///< SqlResult for this table
   ColumnInfo*           _pColumnsInfo; ///< Column metadata info array
   ColumnMap<SqlResult>* _pColumnMaps;  ///< Column maps array
   Row*                  _pSavedRow;    ///< Original row of a changed row
   std::vector<Row*>     _rows;         ///< Cached data backend
   std::string           _tableName;    ///< sql table name

   /** 
    * \brief SQL command for this table.
    * This value can be set in constructor or reloadData();
    * If _sqlCommandToRun not empty string and != "dummy",
    * init() will copy it into _query, and runs that query\n
    * For example: we can set "SELECT data FROM table WHERE id = 1"; in constructor\n
    */
   std::string _sqlCommandToRun;

   bool _setReadOnly;                   ///< Is table read only
   bool _colLabelWithType;              ///< Should column label also show column type?
   bool _valid;                         ///< The Table is filled with data and the data is OK

   std::string _query;                  ///< Sql query run by init()

   long _numColumns;                    ///< Total column count.
   int _lastModifiedRow;                ///< Last modified row
   int _lastInsertedRow;                ///< Last Inserted row;
   int _rowsCached;                     ///< Rows read from dataset. if nRows=rowsCached, _pResult can be deleted
   int _rowsAdded;                      ///< Rows added (never been in dataSet).
   int _rowsDeleted;                    ///< Rows deleted from initial dataSet.
   int _lastSuccessfullyDeletedRows;    ///< Last successfully deleted rows count;
   long _initialRows;                   ///< Initial rows count after init()
   long _totalRows;                     ///< Total rows in this table (existing and new)
   bool _changed;                       ///<

   long _optionPageSize;                ///< Total rows in a page/paging
   long _pageCount;                     ///< Total page count

   /// Options for retrieing data from backend.
   TableRetrieveDataOption _retrieveDataOption;

   /// Implementation helper class.
   TableHelperImpl _helperImpl;

   /// Table navigator.
   Navigator _navigator;

   /// Implementation logger class.
   LoggerImpl _logger;
};


} // namespace sql
} // namespace tbs