#pragma once

#include <tobasa/exception.h>

namespace tbs 
{

/** 
 * Exception generated validation check
 */
class ValidationException
   : public tbs::AppException
{
public:

   ValidationException(const std::string& msg, const std::string& src = "")
      : tbs::AppException(msg.c_str(), src)
   {}
};


} //namespace tbs