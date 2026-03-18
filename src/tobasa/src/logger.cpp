#include "tobasa/logger.h"

namespace tbs {

// Default log sink
std::unique_ptr<log::LogSink> Logger::_pTarget = std::make_unique<log::CoutLogSink>();

void Logger::enableLogging()
{
   _enableLog = true;
}

void Logger::disableLogging()
{
   _enableLog = false;
}

std::string Logger::logFilePath()
{
   return _logFilePath;
}

void Logger::logFilePath(const std::string& logFilePath)
{
   _logFilePath = path::absolute(logFilePath);
}

void Logger::destroyInternalLogSink()
{
   setTarget(nullptr);
}

void Logger::setTarget(log::LogSink* newTarget)
{
   _pTarget.reset(newTarget);
}

void Logger::logT(const std::string& message)
{
   if (_enableLog && _pTarget)
      _pTarget->trace(message);
}

void Logger::logD(const std::string& message)
{
   if (_enableLog && _pTarget)
      _pTarget->debug(message);
}

void Logger::logI(const std::string& message)
{
   if (_enableLog && _pTarget)
      _pTarget->info(message);
}

void Logger::logW(const std::string& message)
{
   if (_enableLog && _pTarget)
      _pTarget->warn(message);
}

void Logger::logE(const std::string& message)
{
   if (_enableLog && _pTarget)
      _pTarget->error(message);
}

} // namespace tbs