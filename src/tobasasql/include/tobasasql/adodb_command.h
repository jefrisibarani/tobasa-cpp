#pragma once

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#import "c:\Program Files\Common Files\System\ado\msado15.dll" rename("EOF", "EndOfFile")

#include <tobasa/self_counter.h>
#include <tobasa/notifier.h>
#include "tobasasql/adodb_common.h"
#include "tobasasql/com_variant_helper.h"

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief ADO command class.
 */
class AdoCommand : public Notifier
{
public:
   using VariantHelper = ComVariantHelper;

   AdoCommand(ADODB::_ConnectionPtr pConn);
   ~AdoCommand() = default;

   /**
    * \brief Execute sql command or stored procedure that does not return rows.
    * \details On successfull execution, returns true
    * On error, std::exception thrown
    * On bad connection, retuns false
    * 
    * Result from sql command returning row(s) is ignored and affected rows is 0.
    * 
    * \param[in]  sql           Sql command query
    * \param[out] affectedRows  Record(s) affected by executed sql command
    * \param[in]  parameters    AdoParameter collection.
    */
   bool execute(
      const std::string& sql,
      int&  affectedRows,
      const AdoParameterCollection& parameters = AdoParameterCollection());

   /**
    * \brief Execute sql query and return ADODB::_RecordsetPtr.
    * \param[in]  sql           Sql command query
    * \param[in]  parameters    AdoParameter collection.
    */
   ADODB::_RecordsetPtr executeResult(
      const std::string& sql,
      const AdoParameterCollection& parameters = AdoParameterCollection());

   /// Create ADODB::_ParameterPtr object from AdoParameter object.
   ADODB::_ParameterPtr createParameter( AdoParameter& param );

   /// Create native ADODB Command object.
   ADODB::_CommandPtr createNativeCommand(const std::string& sql, const AdoParameterCollection& parameters);

private:
   ADODB::_ConnectionPtr _pConn;
};

} // namespace sql
} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)