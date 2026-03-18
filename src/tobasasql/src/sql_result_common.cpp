#include "tobasasql/exception.h"
#include "tobasasql/util.h"
#include "tobasa/util_string.h"
#include "tobasasql/sql_result_common.h"

namespace tbs {
namespace sql {

ResultCommon::ResultCommon()
   : _resultStatus { ResultStatus::unknown } {}

int ResultCommon::affectedRows() const
{
   return _affectedRows;
}

ResultStatus ResultCommon::resultStatus() const
{
   return _resultStatus;
}

std::string ResultCommon::getSqlQuery() const
{
   return _qryStr;
}

bool ResultCommon::isValid() const
{
   return  (_resultStatus == ResultStatus::commandOk) || (_resultStatus == ResultStatus::tuplesOk);
}

void ResultCommon::setOptionOpenTable(bool openTable)
{
   _optionOpenTable = openTable;
}

void ResultCommon::setOptionCacheData(bool cache)
{
   _optionCacheData = cache;
}

long ResultCommon::totalRows() const
{
   return _nRows;
}

long ResultCommon::totalColumns() const
{
   return _nColumns;
}

std::vector<std::string> ResultCommon::columnNames() const
{
   std::vector<std::string> colNameCollection;
   for(size_t i=0;i<_columnInfoCollection.size();i++)
   {
      colNameCollection.push_back(_columnInfoCollection.at(i).name);
   }

   return colNameCollection;
}

std::string ResultCommon::columnName(const int columnIndex) const
{
   throwIfColumnIndexInvalid(columnIndex);
   throwIfColumnInfoInvalid();

   return _columnInfoCollection[columnIndex].name;
}

int ResultCommon::columnNumber(const std::string& name) const
{
   if ( _nColumns == 0 )
      throw SqlException("invalid columns size ", "SqlResult");

   for (int i = 0; i < _nColumns; i++)
   {
      if (util::toLower(columnName(i)) == util::toLower(name))
      {
         int colNumber = i;
         return colNumber;
      }
   }

   throw SqlException("invalid column name ", "SqlResult");
}

std::string ResultCommon::columnNativeTypeStr(const int columnIndex) const
{
   throwIfColumnIndexInvalid(columnIndex);
   throwIfColumnInfoInvalid();

   return _columnInfoCollection[columnIndex].nativeTypeStr;
}

std::string ResultCommon::columnNativeFullTypeStr(const int columnIndex) const
{
   throwIfColumnIndexInvalid(columnIndex);
   throwIfColumnInfoInvalid();

   return _columnInfoCollection[columnIndex].nativeFullTypeStr;
}

long ResultCommon::columnNativeType(const int columnIndex) const
{
   throwIfColumnIndexInvalid(columnIndex);
   throwIfColumnInfoInvalid();

   return _columnInfoCollection[columnIndex].nativeType;
}

DataType ResultCommon::columnDataType(const int columnIndex) const
{
   throwIfColumnIndexInvalid(columnIndex);
   throwIfColumnInfoInvalid();

   return _columnInfoCollection[columnIndex].dataType;
}

long ResultCommon::columnDefinedSize(const int columnIndex) const
{
   throwIfColumnIndexInvalid(columnIndex);
   throwIfColumnInfoInvalid();

   return _columnInfoCollection[columnIndex].definedSize;
}

int ResultCommon::columnNumericScale(const int columnIndex) const
{
   throwIfColumnIndexInvalid(columnIndex);
   throwIfColumnInfoInvalid();

   return _columnInfoCollection[columnIndex].numericScale;
}

short ResultCommon::columnPrecision(const int columnIndex) const
{
   throwIfColumnIndexInvalid(columnIndex);
   throwIfColumnInfoInvalid();

   return _columnInfoCollection[columnIndex].precision;
}

bool ResultCommon::columnIsPrimaryKey(const int columnIndex) const
{
   throwIfColumnIndexInvalid(columnIndex);
   throwIfColumnInfoInvalid();

   return _columnInfoCollection[columnIndex].primaryKey;
}

bool ResultCommon::columnIsAutoIncrement(const int columnIndex) const
{
   throwIfColumnIndexInvalid(columnIndex);
   throwIfColumnInfoInvalid();

   return _columnInfoCollection[columnIndex].autoIncrement;
}

void ResultCommon::throwIfColumnIndexInvalid(const int columnIndex, const std::string& source) const
{
   if ((_nColumns == 0) || columnIndex > _nColumns || columnIndex < 0 ) {

      std::string msgSource = "SqlResult";
      if (!source.empty()) {
         msgSource = source;
      }

      throw SqlException("invalid column index", msgSource);
   }
}

void ResultCommon::throwIfRowIndexInvalid(const int currentRow, const std::string& source) const
{
   if ( _nRows == 0 || currentRow > _nRows || currentRow < 0)
   {
      std::string msgSource = "SqlResult";
      if (!source.empty()) {
         msgSource = source;
      }

      throw SqlException("invalid row index", msgSource);
   }
}

void ResultCommon::throwIfColumnInfoInvalid(const std::string& source) const
{
   if ( _columnInfoCollection.empty() )
   {
      std::string msgSource = "SqlResult";
      if (!source.empty()) {
         msgSource = source;
      }

      throw SqlException("invalid field info collection", msgSource);
   }
}


} // namespace sql
} // namespace tbs