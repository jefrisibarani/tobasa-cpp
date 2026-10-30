#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "tobasa/logger.h"
#include "tobasa/logger_sink_multiple.h"

namespace tbs {
namespace log {

namespace {
spdlog::level::level_enum fromTobasaLogLevel(LogLevel level)
{
   if (level == LogLevel::trace)     return spdlog::level::level_enum::trace;
   if (level == LogLevel::debug)     return spdlog::level::level_enum::debug;
   if (level == LogLevel::info)      return spdlog::level::level_enum::info;
   if (level == LogLevel::warn)      return spdlog::level::level_enum::warn;
   if (level == LogLevel::err)       return spdlog::level::level_enum::err;
   if (level == LogLevel::critical)  return spdlog::level::level_enum::critical;
   if (level == LogLevel::off)       return spdlog::level::level_enum::off;
   if (level == LogLevel::n_levels)  return spdlog::level::level_enum::n_levels;
   throw std::runtime_error("invalid LogLevel");
}
} // namespace


struct MultipleLogSink::LogImpl
{
   std::shared_ptr<spdlog::logger> spdLogger = nullptr;
};


MultipleLogSink::MultipleLogSink(const conf::Logging& option)
{
   _logImpl = std::make_unique<MultipleLogSink::LogImpl>();

   spdlog::flush_every(std::chrono::seconds(1));

   // Always have a console sink
   auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
   consoleSink->set_level(fromTobasaLogLevel(option.stdoutColor.level));
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
         fileSink->set_level(fromTobasaLogLevel(option.fileSink.level));
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
         fileSinkD->set_level(fromTobasaLogLevel(option.fileSinkD.level));
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
   newDefLogger->set_level(fromTobasaLogLevel(option.multiSinkLevel));

   //_logImpl->spdLogger = spdlog::get("multi_sink");
   _logImpl->spdLogger = newDefLogger;
}

MultipleLogSink::~MultipleLogSink() 
{
   _logImpl->spdLogger.reset(); 
   spdlog::drop("multi_sink");
}

void MultipleLogSink::trace(const std::string& message)
{
   _logImpl->spdLogger->trace(message);
}

void MultipleLogSink::debug(const std::string& message)
{
   _logImpl->spdLogger->debug(message);
}

void MultipleLogSink::info(const std::string& message)
{
   _logImpl->spdLogger->info(message);
}

void MultipleLogSink::warn(const std::string& message)
{
   _logImpl->spdLogger->warn(message);
}

void MultipleLogSink::error(const std::string& message)
{
   _logImpl->spdLogger->error(message);
}

} // namespace log
} // namespace tbs