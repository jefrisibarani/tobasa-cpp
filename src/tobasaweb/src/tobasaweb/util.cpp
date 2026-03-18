#include <tobasa/config.h>
#include <tobasa/exception.h>
#include <tobasa/uuid.h>
#include <tobasa/crypt.h>
#include <tobasa/util.h>
#include <tobasa/util_string.h>
#include "tobasaweb/credential_info.h"
#include "tobasaweb/util.h"

namespace tbs {
namespace util {


sql::SqlQueryOption getSqlQueryOption(
   const web::RouteArgument& arg,
   std::string& outStartDate,
   std::string& outEndDate,
   std::string& outErrorMessage)
{
   auto httpCtx     = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   auto query       = httpCtx->request()->query();
   bool hasError    = false;

   long qOffset, qLimit;
   std::string qStartDate, qEndDate;
   qOffset = qLimit = -1;

   if ( query->hasField("offset") )
   {
      auto offset = query->value("offset");
      if (util::isNumber(offset))
         qOffset = std::stol(offset);
      else
      {
         hasError = true;
         outErrorMessage = "Invalid parameter offset";
      }
   }

   if ( query->hasField("limit") )
   {
      auto limit = query->value("limit");
      if (util::isNumber(limit))
         qLimit = std::stol(limit);
      else
      {
         hasError = true;
         outErrorMessage = "Invalid parameter limit";
      }
   }

   if ( query->hasField("startdate") )
   {
      auto sdate = query->value("startdate");
      if (!sdate.empty())
         qStartDate = sdate;
      else
      {
         hasError = true;
         outErrorMessage = "Invalid parameter start date";
      }
   }

   if ( query->hasField("enddate") )
   {
      auto edate = query->value("enddate");
      if (!edate.empty())
         qEndDate = edate;
      else
      {
         hasError = true;
         outErrorMessage = "Invalid parameter end date";
      }
   }

   // date filter not defined, set default date filter values
   if (qStartDate.empty() && qEndDate.empty() )
   {
      DateTime start;
      start.timePoint() -= tbsdate::days{14};
      qStartDate = start.isoDateString();
      qEndDate   = DateTime::now().isoDateString();
   }

   DateTime dtStart, dtEnd;
   if (dtStart.parse(qStartDate,"%Y-%m-%d") && dtEnd.parse(qEndDate,"%Y-%m-%d") )
   {
      if (dtStart.timePoint() > dtEnd.timePoint() )
      {
         hasError = true;
          outErrorMessage = "Start date cannot be later than end date";
      }
   }
   else
   {
      hasError = true;
      outErrorMessage = "Could not parse parameter date";
   }

   // update outStartDate and outEndDate to the caller
   outStartDate = qStartDate;
   outEndDate   = qEndDate;

   std::string filter;
   if ( query->hasField("filter") )
   {
      filter = query->value("filter");
   }

   sql::SqlQueryOption option(qLimit, qOffset, filter);
   option.startDate = qStartDate;
   option.endDate   = qEndDate;
   option.hasInvalidParameter = hasError;
   return std::move(option);
}


void createSecureHash(const std::string& message,std::string& messageHashOut, std::string& saltHashOut)
{
   using namespace crypt;
   auto randomStr = uuid::generate();
   saltHashOut    = hashSHA(ShaType::SHA256, randomStr);
   messageHashOut = hmacSHA(ShaType::SHA512, message, saltHashOut, true);
}

bool verifySecureHash(const std::string& message, const std::string& storedMessageHash, const std::string& storedSaltHash)
{
   if (message.empty())
      throw AppException("empty message", "verifySecureHash");

   using namespace crypt;
   std::string passwordHmacHash = hmacSHA(ShaType::SHA512, message, storedSaltHash, true);
   if (passwordHmacHash == util::toUpper(storedMessageHash)) {
      return true;
   }

   return false;
}

} // namespace util
} // namespace tbs