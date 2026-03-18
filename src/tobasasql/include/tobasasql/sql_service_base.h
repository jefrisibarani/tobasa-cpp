#pragma once

#include <string>
#include <memory>
#include <functional>
#include <tobasa/non_copyable.h>

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */

struct SqlServiceInfo
{
   int id;
   std::string name;
};

class DatabaseConnector;

/**
 * SqlServiceBase.
 */
class SqlServiceBase : private NonCopyable
{
   friend class DatabaseConnector;
   
public:
   SqlServiceBase()
   {
      ++_id;
      _selfId = _id;
   }

   virtual ~SqlServiceBase() = default;

   int id() { return _selfId; }
   std::string name() { return _name; }

   SqlServiceInfo info()
   {
      SqlServiceInfo info;
      info.id = _selfId;
      info.name = _name;
      return info;
   }

   virtual bool beginTransaction()
   {
      if (beginTransactionCallback)
         return beginTransactionCallback();

      return true;
   }

   virtual bool commitTransaction()
   {
      if (commitTransactionCallback)
         return commitTransactionCallback();

      return true;
   }

   virtual bool rollbackTransaction() 
   {
      if (rollbackTransactionCallback)
         return rollbackTransactionCallback();

      return true;
   }

   virtual bool databaseConnected()
   {
      if (databaseConnectedCallback)
         return databaseConnectedCallback();

      return true;
   }

protected:

   void setName(const std::string& name)
   {
      _name = name;
   }

   std::function<bool()> beginTransactionCallback;
   std::function<bool()> commitTransactionCallback;
   std::function<bool()> rollbackTransactionCallback;
   std::function<bool()> databaseConnectedCallback;

private:
   std::string _name;
   int _selfId = 0;
   inline static int _id = 1;
};

using SqlServicePtr = std::shared_ptr<SqlServiceBase>;

/** @}*/

} // namespace sql
} // namespace tbs