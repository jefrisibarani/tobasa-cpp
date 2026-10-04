#pragma once

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#import "c:\Program Files\Common Files\System\ado\msado15.dll" rename("EOF", "EndOfFile")

#include <tobasa/self_counter.h>
#include <tobasa/notifier.h>
#include "tobasasql/adodb_common.h"
#include "tobasasql/com_variant_helper.h"

namespace tbs {
namespace sql {

template <typename VariantTypeImplemented>
class DataSet;

class AdodbConnection;
class AdodbResult;

/** 
 * \ingroup SQL
 * \brief ADO command class.
 */
class AdodbCommand : public Notifier
{
public:

   using VariantType   = ComVariantType;
   using VariantHelper = ComVariantHelper;
   using VectorVariant = std::vector<VariantType>;
   using RecordVariant = std::vector<VectorVariant>;
   using DataSetPtr    = std::shared_ptr<DataSet<VariantType>>;

   AdodbCommand(AdodbConnection* conn);
   ~AdodbCommand() = default;

   /// Prepare for one-shot query execution
   bool query(const std::string& sql, const AdoParameterCollection& parameters);

   bool prepare(const std::string& sql);
   bool bind(const AdoParameterCollection& parameters);
   void reset();
   void close();

   /**
    * @brief Executes the prepared query.
    * @return - The number of affected rows for INSERT, UPDATE, or DELETE
    *         - Zero for SELECT and other queries that do not affect rows.
    *         - -1 if invalid operation occured
    * Throwing Exception when error occured
    */
   int execute();

   /**
    * @brief Executes the prepared query.
    * @return - The first column of the first retrieved row, as std::string
    *         - Returned value may be empty string
    * Throwing Exception when error occured
    */
   std::string executeScalar();

   /**
    * @brief Executes the prepared query and returns its result set.
    * @return A shared dataset for row-producing queries such as SELECT;
    *         an empty shared pointer for INSERT, UPDATE, or DELETE.
    * Throwing Exception when error occured
    */
   DataSetPtr executeResult();

   AdodbResult executeSqlResult();

   ADODB::_RecordsetPtr executeRecordsetPtr();

   int affectedRows() { return _affectedRows; }

   std::string sqlCommandText() const { return _sql; }

   // --------------------------------------------------------------------------
   /// Create ADODB::_ParameterPtr object from AdoParameter object.
   ADODB::_ParameterPtr createParameter( AdoParameter& param );

   static std::string getScalarResult(ADODB::_RecordsetPtr recordset, long& totalRows);

   /// Create native ADODB Command object.
   ADODB::_CommandPtr createNativeCommand(const std::string& sql, const AdoParameterCollection& parameters);

private:

   AdodbConnection*      _pConn;
   ADODB::_ConnectionPtr _pAdoConn;
   ADODB::_CommandPtr    _pAdoCommand;

   // Native ADO parameters cache.
   std::vector<ADODB::_ParameterPtr> _parameters;
   std::string _sql;

   int    _affectedRows;
   std::string lastBackendError() { return ""; }
   std::string statementError()   { return ""; }
   
   //void clearNativeParameters();

};

} // namespace sql
} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)