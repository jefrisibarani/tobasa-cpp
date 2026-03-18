#pragma once

#include <tobasa/notifier.h>
#include <tobasa/self_counter.h>
#include "tobasasql/common_types.h"

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief Sql result common.
 * Abstact base class serves as a root class for all implemented sql result
*/
class ResultCommon : public Notifier
{
public:
   ResultCommon();
   virtual ~ResultCommon() = default;

   // -------------------------------------------------------
   // Implemented in implementation class:
   //
   // name()
   // runQuery()
   // set connection()
   // get connection()
   // columnTypeClass(),
   // getVariantValue()
   // getStringValue()
   // isNullField()
   // -------------------------------------------------------

   /// Get affected rows.
   virtual int affectedRows() const;

   /// Get result status.
   ResultStatus resultStatus() const;

   /// Get stored sql query.
   std::string getSqlQuery() const;

   /// Check is this result valid.
   bool isValid() const;

   /// Set operation to open table instead of running sql command.
   /// runQuery() option, get data from table or from sql command.
   virtual void setOptionOpenTable(bool openTable = true);

   /// Set operation to cache data from database.
   /// runQuery() option, cache retrieved data from backend.
   /// Result retrieved from database will be cached if this option true
   virtual void setOptionCacheData(bool cache = true);

   /// Get total rows.
   long totalRows() const;

   /// Get total columns.
   long totalColumns() const;

   /// Get column names list.
   std::vector<std::string> columnNames() const;

   /// Get column name.
   std::string columnName(const int columnIndex) const;

   /// Get column position by name.
   int columnNumber(const std::string& name) const;

   /// Get Sql column native as string.
   std::string columnNativeTypeStr(const int columnIndex) const;

   /// Get Sql column native as string.
   std::string columnNativeFullTypeStr(const int columnIndex) const;

   /// Get Sql column native type.
   long columnNativeType(const int columnIndex) const;

   /// Get Sql Data type.
   DataType columnDataType(const int columnIndex) const;

   /// Get Defined column size/length.
   long columnDefinedSize(const int columnIndex) const;

   /// Get Defined column size/length.
   int columnNumericScale(const int columnIndex) const;

   /// Get Defined column size/length.
   short columnPrecision(const int columnIndex) const;
   
   virtual bool columnIsPrimaryKey(const int columnIndex) const;

   virtual bool columnIsAutoIncrement(const int columnIndex) const;
protected:

   void throwIfColumnIndexInvalid(const int columnIndex, const std::string& source = "") const;

   void throwIfRowIndexInvalid(const int currentRow, const std::string& source = "") const;

   void throwIfColumnInfoInvalid(const std::string& source = "") const;

   ResultStatus _resultStatus;
   std::string  _qryStr;
   int _nColumns   = 0;
   int _nRows      = 0;

   /// Column informations.
   struct ColumnInfo
   {
      std::string name;
      long        nativeType;
      std::string nativeTypeStr;
      std::string nativeFullTypeStr;
      DataType    dataType          = DataType::unknown;
      long        definedSize       = 0;
      bool        autoIncrement     = false;
      int         numericScale      = 0;  
      short       precision         = 0;
      bool        primaryKey        = false;
      bool        allowNull         = false;
   };

   /// Column information collection.
   std::vector<ColumnInfo> _columnInfoCollection;

   /// Affected row(s) by INSERT/DELETE/UPDATE query
   int _affectedRows = 0;

   /** 
    * runQuery() option, get data from table or from sql command.
    * Only table name may be passed to runQuery()
    */
   bool _optionOpenTable = false;

   /** 
    * runQuery() option, cache retrieved data from backend.
    * Result retrieved from database will be cached if this option true
    */
   bool _optionCacheData = false;
};

} // namespace sql
} // namespace tbs