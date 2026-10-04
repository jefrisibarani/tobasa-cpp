#include <stdexcept>
#include "tobasasql/sql_connection_common.h"

namespace tbs {
namespace sql {

ConnectionCommon::ConnectionCommon() noexcept
   : _connStatus { ConnectionStatus::bad }
   , _logIdentifier()
   , _logSqlQuery(true)
   , _logExecuteStatus(true)
   , _logSqlQueryInternal(false)
{
}

ConnectionCommon::ConnectionCommon(ConnectionCommon&& other) noexcept
   : _connStatus(          other._connStatus)
   , _logIdentifier(       std::move(other._logIdentifier))
   , _logSqlQuery(         other._logSqlQuery)
   , _logExecuteStatus(    other._logExecuteStatus)
   , _logSqlQueryInternal( other._logSqlQueryInternal)
{
   other._connStatus          = ConnectionStatus::bad;
   other._logIdentifier.clear();
   other._logSqlQuery         = true;
   other._logExecuteStatus    = true;
   other._logSqlQueryInternal = false;
}

ConnectionCommon& ConnectionCommon::operator=(ConnectionCommon&& other) noexcept
{
   if (this != &other)
   {
      _connStatus          = other._connStatus;
      _logIdentifier       = std::move(other._logIdentifier);
      _logSqlQuery         = other._logSqlQuery;
      _logExecuteStatus    = other._logExecuteStatus;
      _logSqlQueryInternal = other._logSqlQueryInternal;

      other._connStatus          = ConnectionStatus::bad;
      other._logIdentifier.clear();
      other._logSqlQuery         = true;
      other._logExecuteStatus    = true;
      other._logSqlQueryInternal = false;
   }
   return *this;
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