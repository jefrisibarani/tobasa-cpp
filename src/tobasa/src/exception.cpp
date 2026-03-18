#include <cstring>
#include "tobasa/format.h"
#include "tobasa/exception.h"

namespace tbs {

AppError::AppError()
   : line(0)
{}

AppError::AppError(const std::string msg, const std::string& src, const char* fl, int ln)
{
   message = msg;
   source  = src;
   file    = fl;
   line    = ln;

   if ( strlen(fl)>0 )
      fullMessage = tbsfmt::format("Message: {}, Source: {}, File: {}, Line: {}", message, source, file, line);
   else
      fullMessage = tbsfmt::format("Message: {}, Source: {}", message, source);
}


AppException::AppException(const AppError& appErr)
   : std::runtime_error(appErr.message)
{
   appError = appErr;
}

AppException::AppException(const std::string& msg, const std::string& src, const char* fl, int ln)
   : std::runtime_error(msg.c_str())
{
   appError = AppError(msg, src, fl, ln);
}

AppException::AppException(const char* msg, const std::string& src, const char* fl, int ln)
   : std::runtime_error(msg)
{
   appError = AppError(msg, src, fl, ln);
}

AppException::AppException(const std::exception &ex, const std::string& src, const char* fl, int ln)
   : std::runtime_error(ex.what())
{
   appError = AppError(ex.what(), src, fl, ln);
}


} // namespace tbs