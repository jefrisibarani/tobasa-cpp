#pragma once

#include <tobasa/self_counter.h>
#include <tobasa/notifier.h>
#include "tobasasql/common_types.h"

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */

/**
 * \brief Sql connection base.
 * Class serves as a root class for all implemented sql connection
 */
class ConnectionCommon : public Notifier
{
public:
   /// Constructor.
   ConnectionCommon();

   /// Destructor.
   ~ConnectionCommon();

   void setLogSqlQuery(bool enable = true);
   bool logSqlQuery();

   void setLogExecuteStatus(bool enable = true);
   bool logExecuteStatus();

   void setLogSqlQueryInternal(bool enable = true);
   bool logSqlQueryInternal();

   void setLogId(const std::string& logId);
   std::string logId();

protected:
   bool checkStatus();

   ConnectionStatus     _connStatus;
   std::string          _logIdentifier;
   bool _logSqlQuery          = true;
   bool _logExecuteStatus     = true;
   bool _logSqlQueryInternal  = false;
};

/// Temporarily apply sql logging.
class SqlApplyLogInternal
{
public:
   SqlApplyLogInternal(ConnectionCommon* pConn);
   ~SqlApplyLogInternal();

private:
   ConnectionCommon* _pConn;
   bool _currentValue;
};

/** @}*/

} // namespace sql
} // namespace tbs