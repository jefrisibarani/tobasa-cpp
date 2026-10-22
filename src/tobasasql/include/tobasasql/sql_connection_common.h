#pragma once

#include "tobasasql/exp_imp.h"
#include <utility>
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
class TBS_SQL_API ConnectionCommon : public Notifier
{
public:
   /// Constructor.
   ConnectionCommon() noexcept;

   /// Copying is disabled because the owning subclasses manage raw native handles.
   ConnectionCommon(const ConnectionCommon&) = delete;
   ConnectionCommon& operator=(const ConnectionCommon&) = delete;

   /// Move constructor.
   ConnectionCommon(ConnectionCommon&& other) noexcept;

   /// Move assignment.
   ConnectionCommon& operator=(ConnectionCommon&& other) noexcept;

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
class TBS_SQL_API SqlApplyLogInternal
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