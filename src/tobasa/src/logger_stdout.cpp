#include <iostream>

#include "tobasa/logger_sink.h"
#include "tobasa/logger_stdout.h"

namespace tbs {
namespace log {

StdoutLogger::StdoutLogger()
{
   _sink = new CoutLogSink();
}

StdoutLogger::~StdoutLogger()
{
   if (_sink)
      delete _sink;
}

void StdoutLogger::logT(const std::string& msg)
{
   _sink->trace(msg);
}

void StdoutLogger::logD(const std::string& msg)
{
   _sink->debug(msg);
}

void StdoutLogger::logI(const std::string& msg)
{
   _sink->info(msg);
}

void StdoutLogger::logW(const std::string& msg)
{
   _sink->warn(msg);
}

void StdoutLogger::logE(const std::string& msg)
{
   _sink->error(msg);
}

} // namespace log
} // namespace tbs