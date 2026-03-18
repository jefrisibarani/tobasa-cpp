#include <tobasa/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "tobasaweb/multi_logger.h"

namespace tbs {
namespace log {

MultiLogger::MultiLogger(const conf::Logging& option)
{
   spdlog::flush_every(std::chrono::seconds(1));

   // Always have a console sink
   auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
   consoleSink->set_level(option.stdoutColor.level);
   consoleSink->set_pattern(option.stdoutColor.pattern);

   std::vector<spdlog::sink_ptr> sinkList;
   sinkList.push_back(consoleSink);

   // Optional file sink
   if (option.fileSink.enable) 
   {
      try 
      {
         auto fileSink = std::make_shared<spdlog::sinks::daily_file_format_sink_mt>(
               option.fileSink.filePath, 23, 59, option.fileSink.truncate);
         fileSink->set_level(option.fileSink.level);
         fileSink->set_pattern(option.fileSink.pattern);
         sinkList.push_back(fileSink);

         Logger::logFilePath(fileSink->filename());
      }
      catch (const spdlog::spdlog_ex& ex) {
         std::cerr << "Failed to create fileSink: " << ex.what() << std::endl;
      }
      catch (...) {
         std::cerr << "Failed to create fileSink: unknown error" << std::endl;
      }
   }

   // Optional second file sink
   if (option.fileSinkD.enable) 
   {
      try 
      {
         auto fileSinkD = std::make_shared<spdlog::sinks::daily_file_format_sink_mt>(
               option.fileSinkD.filePath, 23, 59, option.fileSinkD.truncate);
         fileSinkD->set_level(option.fileSinkD.level);
         fileSinkD->set_pattern(option.fileSinkD.pattern);
         sinkList.push_back(fileSinkD);
      }
      catch (const spdlog::spdlog_ex& ex) {
         std::cerr << "Failed to create fileSinkD: " << ex.what() << std::endl;
      }
      catch (...) {
         std::cerr << "Failed to create fileSinkD: unknown error" << std::endl;
      }
   }

   // Drop existing logger if already registered
   if (spdlog::get("multi_sink"))
      spdlog::drop("multi_sink");

   // Create new multi-sink logger
   auto newDefLogger = std::make_shared<spdlog::logger>("multi_sink", sinkList.begin(), sinkList.end());
   spdlog::set_default_logger(newDefLogger);

   // Apply global log level
   newDefLogger->set_level(option.multiSinkLevel);

   //_pActualLogger = spdlog::get("multi_sink");
   _pActualLogger = newDefLogger;
}

MultiLogger::~MultiLogger() 
{
   _pActualLogger.reset(); 
   spdlog::drop("multi_sink");
}

void MultiLogger::trace(const std::string& message)
{
   _pActualLogger->trace(message);
}

void MultiLogger::debug(const std::string& message)
{
   _pActualLogger->debug(message);
}

void MultiLogger::info(const std::string& message)
{
   _pActualLogger->info(message);
}

void MultiLogger::warn(const std::string& message)
{
   _pActualLogger->warn(message);
}

void MultiLogger::error(const std::string& message)
{
   _pActualLogger->error(message);
}

} // namespace log
} // namespace tbs