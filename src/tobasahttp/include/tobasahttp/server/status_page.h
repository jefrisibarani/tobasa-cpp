#pragma once

#include <memory>
#include <string>
#include <map>

#include "tobasahttp/status_codes.h"

namespace tbs {
namespace http {

/**
 * Status Page Data.
 * Data for Status Page HTML Template 
 */
struct StatusPageData
{
   std::string pageTitle;
   std::string pageBaseUrl;
   std::string statusCode;
   std::string statusMessage;
   std::string statusMessageLong;
};   

/// Get HTML Status Page data 
std::shared_ptr<StatusPageData> statusPageData(HttpStatus status, const std::string& statusMessageLong = "");

/// Get HTML Status Page
std::string statusPageHtml(HttpStatus status, const std::string& statusMessageLong = "");

} // namespace http
} // namespace tbs