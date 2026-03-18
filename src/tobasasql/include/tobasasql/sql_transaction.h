#pragma once

#include "sql_service_base.h"


namespace tbs {
namespace sql {

class SqlTransaction
{
public:
   SqlTransaction(SqlServicePtr service, const std::string& name="")
      : _service {service}
      , _name {name}
      , _committed {false}
      , _started {false}
      , _rolledBack {false}
   {
      _started = _service->beginTransaction();
   }

   void commit()
   {
      _committed = _service->commitTransaction();
   }

   void rollBack()
   {
      _rolledBack = _service->rollbackTransaction();
   }

   ~SqlTransaction()
   {
      if (! ( _committed || _rolledBack) )
         _service->rollbackTransaction();
   }

private:
   SqlServicePtr _service = nullptr;
   std::string _name;
   bool _committed;
   bool _started;
   bool _rolledBack;
};

} // sql
} //tbs