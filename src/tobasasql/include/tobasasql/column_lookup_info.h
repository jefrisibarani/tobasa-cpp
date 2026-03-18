#pragma once

#include <string>
#include <tobasa/logger.h>
#include "tobasasql/util.h"

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief Generate sql query needed to lookup value from other table.
 * \details 
 * This class will generate the correct sql to get all possible value in lookup table.
 * Example:\n
 * Suppose that we have table objects and object has a brand.
 * Instead of store brand name in objects table, we create object_brands table which lists all
 * possibe brand for an object.
 * Then we only store brand_id in objecst table.
 * 
 * Table objects
 * \code
 *   CREATE TABLE `objects`
 *   (
 *      `object_id` INTEGER PRIMARY KEY AUTOINCREMENT,
 *      `object_code` TEXT,
 *      `object_category_id` INTEGER,
 *      `employee_id` INTEGER,
 *      `brand_id` INTEGER,
 *      `status_id` INTEGER,
 *      `serial_number` TEXT,
 *      `po_id` INTEGER,
 *      `summary` TEXT,
 *      `comment` TEXT,
 *      `description` TEXT,
 *      `office_location_id` INTEGER
 *   );
 * \endcode
 * 
 * Table object_brands
 * \code
 *   CREATE TABLE `object_brands`
 *   (
 *      `object_brand_id` INTEGER PRIMARY KEY AUTOINCREMENT,
 *      `object_brand` TEXT
 *   );
 * \endcode
 * 
 * Then in FormObject constructor, we add :
 * \code
 * FormObject::FormObject(...)
 * {
 *    CreateControls();
 * 
 *    // Set _cbBrand(a wxComboBox) data source to brand_id
 *    AddFieldControl(_cbBrand,"brand_id");
 *    // With columnLookups _cbBrand will display brand names
 *    AddColumnLookups("brand_id","object_brands","object_brand_id",true);
 * }
 * \endcode
 */
class ColumnLookupInfo
{
public:
   /** 
    * \brief Constructor.
    * \param referencingColumn , referencing column ( from above example : brand_id , from table objects)
    * \param referencedTable, referenced table ( from above example : object_brands
    * \param orderBy, order by clause ( from above example : object_brand_id
    * \param needMapping, do we need mapping? ( from above example , yes.
    * Because table objects only store brand_id which we need to convert to object_brand.
    */
   ColumnLookupInfo(
      int   colIndex,
      const std::string& sql,
      bool  needMapping = true)
      : _colIndex(colIndex)
      , _referencingColumn("")
      , _sql(sql)
      , _needMapping(needMapping)
   {
      _selectAll  = false;
      _sqlDefined = true;
   }

   ColumnLookupInfo(
      const std::string& referencingColumn,
      const std::string& sql,
      bool  needMapping = true)
      : _colIndex(-1)
      , _referencingColumn(referencingColumn)
      , _sql(sql)
      , _needMapping(needMapping)
   {
      _selectAll  = false;
      _sqlDefined = true;
   }

   ColumnLookupInfo(
      int   colIndex,
      const std::string& referencedTable,
      const std::string& orderBy,
      bool  needMapping = true)
      : _colIndex(colIndex)
      , _referencingColumn("")
      , _referencedTable(referencedTable)
      , _orderBy(orderBy)
      , _needMapping(needMapping)
   {
      _selectAll  = true;
      _sqlDefined = false;
   }

   ColumnLookupInfo(
      const std::string& referencingColumn,
      const std::string& referencedTable,
      const std::string& orderBy,
      bool  needMapping = true)
      : _colIndex(-1)
      , _referencingColumn(referencingColumn)
      , _referencedTable(referencedTable)
      , _orderBy(orderBy)
      , _needMapping(needMapping)
   {
      _selectAll  = true;
      _sqlDefined = false;
   }

   ColumnLookupInfo(
      int   colIndex,
      const std::string& refdColKey,
      const std::string& refdColVal,
      const std::string& referencedTable,
      const std::string& orderBy,
      bool  needMapping = true)
      : _colIndex(colIndex)
      , _referencingColumn("")
      , _refdColKey(refdColKey)
      , _refdColVal(refdColVal)
      , _referencedTable(referencedTable)
      , _orderBy(orderBy)
      , _needMapping(needMapping)
   {
      _selectAll  = false;
      _sqlDefined = false;
   }

   ColumnLookupInfo(
      const std::string& referencingColumn,
      const std::string& refdColKey,
      const std::string& refdColVal,
      const std::string& referencedTable,
      const std::string& orderBy,
      bool  needMapping = true)
      : _colIndex(-1)
      , _referencingColumn(referencingColumn)
      , _refdColKey(refdColKey)
      , _refdColVal(refdColVal)
      , _referencedTable(referencedTable)
      , _orderBy(orderBy)
      , _needMapping(needMapping)
   {
      _selectAll  = false;
      _sqlDefined = false;
   }

   ColumnLookupInfo(
      int   colIndex,
      const std::string& refdColKey,
      const std::string& refdColVal,
      const std::string& referencedTable,
      const std::string& orderBy,
      const std::string& whereClause,
      bool  needMapping = true)
      : _colIndex(colIndex)
      , _referencingColumn("")
      , _refdColKey(refdColKey)
      , _refdColVal(refdColVal)
      , _referencedTable(referencedTable)
      , _orderBy(orderBy)
      , _whereClause(whereClause)
      , _needMapping(needMapping)
   {
      _selectAll = false;
      _sqlDefined = false;
   }

   ColumnLookupInfo(
      const std::string& referencingColumn,
      const std::string& refdColKey,
      const std::string& refdColVal,
      const std::string& referencedTable,
      const std::string& orderBy,
      const std::string& whereClause,
      bool  needMapping = true)
      : _colIndex(-1)
      , _referencingColumn(referencingColumn)
      , _refdColKey(refdColKey)
      , _refdColVal(refdColVal)
      , _referencedTable(referencedTable)
      , _orderBy(orderBy)
      , _whereClause(whereClause)
      , _needMapping(needMapping)
   {
      _selectAll  = false;
      _sqlDefined = false;
   }

   ~ColumnLookupInfo() {}

   std::string getReferencedTable() const { return _referencedTable; }
   std::string getReferencingColumn() const { return _referencingColumn; }
   std::string getOrderByClause() const { return _orderBy; }
   int getColIndex() { return _colIndex; }

   std::string getSql() const
   {
      if (_sqlDefined)
      {
         if (!_sql.empty())
            return _sql;
         else
            return "";
      }

      std::string sql;

      if (_selectAll) {
         sql += "SELECT * FROM " + util::quoteIdent(_referencedTable);
      }
      else
      {
         sql += "SELECT " + util::quoteIdent(_refdColKey) + ", ";
         sql += util::quoteIdent(_refdColVal) + " FROM " + util::quoteIdent(_referencedTable);
      }

      if (!_whereClause.empty()) {
         sql += " WHERE " + _whereClause;
      }

      if (!_orderBy.empty()) {
         sql += " ORDER BY " + _orderBy;
      }

      return sql;
   }

   bool needMapping() const { return  _needMapping; }

private:

   int         _colIndex;
   std::string _referencingColumn;
   std::string _refdColKey;
   std::string _refdColVal;
   std::string _referencedTable;
   std::string _orderBy;
   std::string _whereClause;
   std::string _sql;
   bool        _needMapping;
   bool        _selectAll;
   bool        _sqlDefined;
};

} // namespace sql
} // namespace tbs