#pragma once

#if defined(_MSC_VER) && defined(_WIN32)
#include <windows.h>
#endif

#include <vector>
#include <memory>
#include <sql.h>
#include <sqlext.h>
#include <tobasa/self_counter.h>
#include "tobasasql/sql_parameter.h"

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */

using VariantType = tbs::DefaultVariantType;

/** 
 * \brief ODBC Parameter wrapper.
 * \note Tested only with MS SQL Server
 */
class OdbcParameter
{
public:
   OdbcParameter();
   ~OdbcParameter();

   std::string  name;
   SQLUSMALLINT number;
   SQLSMALLINT  direction;
   SQLSMALLINT  valueType;
   SQLSMALLINT  type;

   /// Precision/number of digits/length in characters
   /// note: https://docs.microsoft.com/en-us/sql/odbc/reference/appendixes/column-size?view=sql-server-ver15
   SQLULEN      columnSize;

   /// Decimal digits or Scale is the number of digits
   // to the right of the decimal point in a number.
   SQLSMALLINT  decimalDigits;
   SQLPOINTER   pValue;
   SQLLEN       bufferLength;
   SQLLEN       strLenIndPtr;

   VariantType  value;
};

/** 
 * \brief ODBC Parameter collection.
 */
class OdbcParameterCollection
{
public:

   OdbcParameterCollection(SQLHSTMT pStmt, short members);

   void prepare(const std::string& sql, const SqlParameterCollection& parameters);

   void bindParameter();

   std::shared_ptr<OdbcParameter> getParam(int pos);

private:

   std::vector<std::shared_ptr<OdbcParameter> > collection;
   SQLHSTMT _pStmt;
};

/** @}*/

} // namespace sql
} // namespace tbs