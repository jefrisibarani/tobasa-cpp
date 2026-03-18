#include <stdexcept>
#include "tobasasql/sql_connection_common.h"

namespace tbs {
namespace sql {

ConnectionCommon::ConnectionCommon()
   : _connStatus { ConnectionStatus::bad }
{
   _logIdentifier = "";
}

ConnectionCommon::~ConnectionCommon() {}

bool ConnectionCommon::checkStatus()
{
   if (_connStatus == ConnectionStatus::bad || 
       _connStatus == ConnectionStatus::ok ||
       _connStatus == ConnectionStatus::refused ||
       _connStatus == ConnectionStatus::dnsError ||
       _connStatus == ConnectionStatus::aborted || 
       _connStatus == ConnectionStatus::broken )
   {
      return true;
   }
   else
      return false;
}

void ConnectionCommon::setLogSqlQuery(bool enable)
{
   _logSqlQuery = enable;
}

bool ConnectionCommon::logSqlQuery()
{
   return _logSqlQuery;
}

void ConnectionCommon::setLogExecuteStatus(bool enable)
{
   _logExecuteStatus = enable;
}

bool ConnectionCommon::logExecuteStatus()
{
   return _logExecuteStatus;
}

void ConnectionCommon::setLogSqlQueryInternal(bool enable)
{
   _logSqlQueryInternal = enable;
}

bool ConnectionCommon::logSqlQueryInternal()
{
   return _logSqlQueryInternal;
}

void ConnectionCommon::setLogId(const std::string& logId)
{
   _logIdentifier = logId;
}

std::string ConnectionCommon::logId()
{
   if (!_logIdentifier.empty() )
      return "[" + _logIdentifier + "] ";
   else
      return {};
}


SqlApplyLogInternal::SqlApplyLogInternal(ConnectionCommon* pConn)
{
   _pConn = pConn;
   _currentValue = _pConn->logSqlQuery();
   bool logInternalSql = _pConn->logSqlQueryInternal();
   _pConn->setLogSqlQuery(logInternalSql);
}

SqlApplyLogInternal::~SqlApplyLogInternal()
{
   // restore back logging setting
   _pConn->setLogSqlQuery(_currentValue);
}


} // namespace sql
} // namespace tbs