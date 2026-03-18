#pragma once

#include <stdexcept>
#include <tobasa/exception.h>

namespace tbs {

/**
 * SQL Exception.
 */
class SqlException : public AppException
{
public:
   SqlException(const AppError& appErr)
      : AppException(appErr)
   {}

   SqlException(const std::string& msg, const std::string& src = "", const char* fl = "", int ln = 0)
      : AppException(msg.c_str(), src, fl, ln)
   {}

   SqlException(const char* msg, const std::string& src = "", const char* fl = "", int ln = 0)
      : AppException(msg, src, fl, ln)
   {}

   SqlException(const std::exception& ex, const std::string& src = "", const char* fl = "", int ln = 0)
      : AppException(ex.what())
   {}
};

} // namespace tbs