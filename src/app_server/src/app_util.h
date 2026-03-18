#pragma once

#include <tobasa/json.h>
#include <tobasaweb/result.h>
#include <tobasahttp/server/status_page.h> //for StatusPageData
#include <tobasasql/settings.h>
#include <tobasaweb/controller_base.h>

namespace tbs {
namespace app {

http::ResultPtr getImageResource(const std::string& imageFile, const std::string& location, const std::string& defaultImage, bool useApiResultOnError=true);

bool copyFileToDisk(const std::string& destFolder, const std::string& fileName, const std::string& sourcePath, std::string &outError);

bool getFileContentFromEnd(const std::string& filePath, int totalLineToRead, Json& out, const std::string& filterVal="ALL");

/// Render Status Page from HTML template and http::StatusPageData object
std::string renderStatusPage(std::shared_ptr<http::StatusPageData> data);

/// Render Status Page from HTML template and http::Result object
std::string renderStatusPage(std::shared_ptr<http::Result> result);

/**
 * Set cookie data for browser
 * Send the new refresh token in HttpOnly cookie
 * Use secure: true and sameSite: 'Strict' → Prevents CSRF attacks.
 */
void setupSessionAndCookie(const http::HttpContext& httpCtx, const std::string& refreshToken);

void logoutAndClearCookie(web::ControllerBase& controller, const http::HttpContext& httpCtx);

bool isValidAppClientId(const std::string& appId);

} // namespace app
} // namespace tbs