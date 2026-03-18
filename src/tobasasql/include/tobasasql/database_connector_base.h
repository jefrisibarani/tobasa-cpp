#pragma once

#include <tobasa/non_copyable.h>
#include <tobasasql/sql_conn_variant.h>

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */

/** 
 * \brief DatabaseConnectorBase
 */
class DatabaseConnectorBase : private NonCopyable
{
protected:
   std::string _name;

public:
   DatabaseConnectorBase() = default;
   virtual ~DatabaseConnectorBase()  = default;

   DatabaseConnectorBase(const std::string& name)
      : _name {name} {}

   virtual bool connect()       = 0;
   virtual bool disconnect()    = 0;
   virtual void initSqlDriver() = 0;
   virtual bool connected()     = 0;

   std::string name() { return _name; }

   virtual sql::SqlConnectionPtrVariant& sqlConnPtrVariant()   = 0;
   virtual sql::SqlDriverVariant&        getSqlDriverVariant() = 0;
};

/// DatabaseConnector shared pointer.
using DatabaseConnectorPtr = std::shared_ptr<DatabaseConnectorBase>;

/** @}*/

} // namespace sql
} // namespace tbs