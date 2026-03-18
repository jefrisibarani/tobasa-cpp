#include <fstream>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <tobasa/path.h>
#include <tobasa/config.h>
#include <tobasa/format.h>
#include <tobasa/util_string.h>
#include <tobasahttp/status_codes.h>
#include <tobasahttp/mimetypes.h>
#include <tobasaweb/settings_webapp.h>
#include <tobasaweb/credential_info.h>
#include <tobasaweb/session.h>
#include <tobasa/util.h>
#include "api_result.h"
#include "app_common.h"
#include "app_util.h"
#include "page.h"
#include "app_resource.h"

namespace tbs {
namespace app {

http::ResultPtr getImageResource(const std::string& imageFile, const std::string& location, const std::string& defaultImage, bool useApiResultOnError)
{
   auto onErrorResult = [](bool useApiResult){
      if (useApiResult)
         return web::apiResult("Image not found", 404, http::StatusCode::NOT_FOUND); 
      else
         return statusResultHtml(http::StatusCode::NOT_FOUND, "Image not found");
   };

   auto webOpt = Config::getOption<web::conf::Webapp>("webapp");

   std::string realLocation;
   if (location      == "IMG_USER" || location == "images_user")
      realLocation = "images_user";
   else if (location == "IMG_CAROUSEL" || location == "images_carousel")
      realLocation = "images_carousel";
   else if (location == "IMG_DOCTOR" || location == "images_doctor")
      realLocation = "images_doctor";
   else
      realLocation = location;

   std::string finalImageFile = defaultImage;
   if (!imageFile.empty())
      finalImageFile = imageFile;

   std::string defaultImgFullPath;
   std::string imgFullPath;

#ifdef TOBASA_BUILD_IN_MEMORY_RESOURCES
   if (web::conf::Webapp::useInMemoryResources)
      defaultImgFullPath = "wwwroot/assets/images/" + defaultImage;
   else
      defaultImgFullPath = app::imageDir() + path::SEPARATOR + defaultImage;
#else
   defaultImgFullPath = app::imageDir() + path::SEPARATOR + defaultImage;
#endif

   imgFullPath = app::dataDir() + path::SEPARATOR + realLocation + path::SEPARATOR + finalImageFile; 
   // If requested image does not exists, user default image
   if (! path::exists(imgFullPath)) 
   {
#ifdef TOBASA_BUILD_IN_MEMORY_RESOURCES
      if (web::conf::Webapp::useInMemoryResources)
      {
         // load static image resource from memory
         if (!app::Resource::exists(defaultImgFullPath, "wwwroot"))  
            return onErrorResult(useApiResultOnError);
         else 
         {
            std::string ext = path::fileExtension(defaultImgFullPath);
            util::strLower(ext);
            auto content = app::Resource::get(defaultImgFullPath, "wwwroot");
            if (content.size() == 0)
               return onErrorResult(useApiResultOnError);
            else
               return http::rawBytesResult(content, http::mimetypes::fromExtension(ext));
         }
      }
      else
      {
         // load static image resource from disk
         if (!path::exists(defaultImgFullPath))  
            return onErrorResult(useApiResultOnError);
         else
         {
            // set requested image with default image 
            imgFullPath = defaultImgFullPath;
         }
      }
#else
      // load static image resource from disk
      if (!path::exists(defaultImgFullPath))  
         return onErrorResult(useApiResultOnError);
      else
      {
         // set requested image with default image 
         imgFullPath = defaultImgFullPath;
      }
#endif
   }
   
   if (! path::exists(imgFullPath))
      return onErrorResult(useApiResultOnError);

   return http::fileResult(imgFullPath);
}

bool copyFileToDisk(const std::string& destFolder, const std::string& fileName, const std::string& sourcePath, std::string &outError)
{
   namespace fs = std::filesystem;
   try
   {
      fs::path source = sourcePath;
      fs::path destination = tbsfmt::format( "{}{}{}", destFolder, path::SEPARATOR, fileName );
      fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
   }
   catch (const std::exception& ex)
   {
      outError = ex.what();
      return false;
   }
   
   return true;
}

bool getFileContentFromEnd(const std::string& filePath, int totalLineToRead, Json& out, const std::string& filterVal)
{
   // Note: https://stackoverflow.com/a/24148653

   std::ifstream ifs(filePath);
   if ( !ifs.good() )
      return false;

   const int k = totalLineToRead;
   std::string* lines = new std::string[k];
   int size = 0 ;

   while ( ifs.good() )
   {
      std::string temp;
      auto idx = size%k;
      std::getline(ifs, temp); // this is just circular array

      if ( temp.length() > 0 )
      {
         if (filterVal == "ALL")
         {
            lines[idx] = temp;
            size++;
         }
         else
         {
            if ( temp.find( filterVal ) != std::string::npos )
            {
               lines[idx] = temp;
               size++;
            }
         }
      }
   }
   ifs.close();

   //start of circular array & size of it
   int start = size > k ? (size%k) : 0 ; //this get the start of last k lines
   int count = std::min(k, size); // no of lines to print

   for (int i = 0; i< count ; i++)
   {
      // start from in between and print from start due to remainder till all counts are covered
      auto idx = (start+i)%k;
      out.emplace_back(lines[idx]);
   }

   delete[] lines;
   return true;
}

std::string renderStatusPage(std::shared_ptr<http::StatusPageData> data)
{
   Json d;
   d["pageTitle"]         = data->pageTitle;
   d["pageBaseUrl"]       = data->pageBaseUrl == "/" ? "" : data->pageBaseUrl;
   d["pageBodyClass"]     = "";
   d["statusCode"]        = data->statusCode;
   d["statusMessage"]     = data->statusMessage;
   d["statusMessageLong"] = data->statusMessageLong;

   auto view = std::make_shared<web::View>();
   view->setData(d);
   return view->render("status_page.tpl");
}

std::string renderStatusPage(std::shared_ptr<http::Result> result)
{
   auto res = std::static_pointer_cast<http::StatusResult>(result);
   std::string statusMessageLong;
   if (res->message().empty())
   {
      if (res->httpStatus().code() == http::StatusCode::UNAUTHORIZED)
         statusMessageLong = "Access to this resource is denied";
      else if (res->httpStatus().code() == http::StatusCode::NOT_FOUND)
         statusMessageLong = "This requested URL was not found on this server";
      else if (res->httpStatus().code() == http::StatusCode::INTERNAL_SERVER_ERROR)
         statusMessageLong = "Internal server error";
   }
   else
      statusMessageLong = res->message();

   auto data = http::statusPageData(res->httpStatus(), statusMessageLong);

   return renderStatusPage(data);
}

void setupSessionAndCookie(const http::HttpContext& httpCtx, const std::string& refreshToken)
{
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   auto session     = web::Session::get(httpCtx->sessionId());
   auto webOpt      = Config::getOption<web::conf::Webapp>("webapp");

   // Cookie expiration time
   auto sessExpireMinutes = webOpt.webService.sessionExpirationMinutes;
   sessExpireMinutes      = (sessExpireMinutes <= 0) ? 10 : sessExpireMinutes ; // default ten minutes
   auto nowDt             = DateTime::now();
   auto sessExpireTp      = nowDt.timePoint() + std::chrono::minutes{ sessExpireMinutes };
   auto sessExpireDt      = DateTime(sessExpireTp);
   
   //authResult.expirationTime = expiredDt.toUnixTimeSeconds() - (floor<std::chrono::seconds>(nowUtc)).time_since_epoch().count();
   authResult.expirationTime = sessExpireDt.toUnixTimeSeconds();

   // Date format : <day-name>, <day> <month> <year> <hour>:<minute>:<second> GMT
   // Example     : Wed, 21 Oct 2015 07:28:00 GMT
   auto sessExpireStr = sessExpireDt.format("{:%a, %d %b %Y %H:%M:%S GMT}", false, true);
   
   // session cookie maximum age in seconds
   auto sessMaxAgeSec    = sessExpireMinutes * 60;
   auto sessMaxAgeSecStr = std::to_string(sessMaxAgeSec);

   //std::string refreshToken        = authDbRepo->generateRefreshToken(authResult.identity.pUser);
   auto refreshTokenExpiredMinutes = webOpt.webService.authJwtRefreshExpireTimeSpanMinutes;
   refreshTokenExpiredMinutes      = (refreshTokenExpiredMinutes <= 0) ? 60*24 : refreshTokenExpiredMinutes; // default 24 hours
   auto refreshTokenExpiredSec     = refreshTokenExpiredMinutes * 60 ;
   auto refreshTokenExpiredStr     = std::to_string(refreshTokenExpiredSec);
   
   httpCtx->response()->addHeader("Set-Cookie", tbsfmt::format("{}={};                    Path=/; Max-Age={}; Secure; SameSite=Strict", web::Session::COOKIE_NAME, httpCtx->sessionId(), sessMaxAgeSecStr));
   httpCtx->response()->addHeader("Set-Cookie", tbsfmt::format("tbs_api_refresh_token={}; Path=/; Max-Age={}; Secure; SameSite=Strict; HttpOnly; ", refreshToken, refreshTokenExpiredStr));
   httpCtx->response()->addHeader("Set-Cookie", tbsfmt::format("tbs_user_name={};         Path=/; Max-Age={}; Secure; SameSite=Strict", authResult.identity.pUser->userName, sessMaxAgeSecStr));
   httpCtx->response()->addHeader("Set-Cookie", tbsfmt::format("tbs_user_uuid={};         Path=/; Max-Age={}; Secure; SameSite=Strict", authResult.identity.pUser->uuid,     sessMaxAgeSecStr));
   httpCtx->response()->addHeader("Set-Cookie", tbsfmt::format("tbs_selected_site_id={};  Path=/; Max-Age={}; Secure; SameSite=Strict", authResult.identity.selectedSiteId,  sessMaxAgeSecStr));
   httpCtx->response()->addHeader("Set-Cookie", tbsfmt::format("tbs_selected_lang_id={};  Path=/; Max-Age={}; Secure; SameSite=Strict", authResult.identity.selectedLangId,  sessMaxAgeSecStr));

   session->setData("expires",          sessExpireDt.toUnixTimeSeconds());
   session->setData("expires_str",      sessExpireDt.isoDateTimeStringUTC());
   session->setData("logged_in",        true);
   session->setData("user_name",        authResult.identity.pUser->userName);
   session->setData("user_id",          authResult.identity.pUser->id);
   session->setData("selected_site_id", authResult.identity.selectedSiteId);
   session->setData("selected_lang_id", authResult.identity.selectedLangId);

   // save client ip address
   std::ostringstream ostr;
   ostr << httpCtx->remoteEndpoint() ;
   session->setData("remote_endpoint", ostr.str() );
}

void logoutAndClearCookie(web::ControllerBase& controller, const http::HttpContext& httpCtx)
{
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   authResult.credentialsProvided = false;
   authResult.credentialsValid = false;
   authResult.identity = {};

   // Delete session storage
   web::Session::destroy(httpCtx->sessionId());

   // Reset browser session cookies
   httpCtx->response()->addHeader("Set-Cookie", web::Session::COOKIE_NAME + "=; Max-Age=0;");
   httpCtx->response()->addHeader("Set-Cookie", "tbs_api_access_token=; Max-Age=0;");
   httpCtx->response()->addHeader("Set-Cookie", "tbs_api_refresh_token=; Max-Age=0;");
   httpCtx->response()->addHeader("Set-Cookie", "tbs_user_name=; Max-Age=0;");
   httpCtx->response()->addHeader("Set-Cookie", "tbs_user_uuid=; Max-Age=0;");
   httpCtx->response()->addHeader("Set-Cookie", "tbs_selected_site_id=; Max-Age=0;");
   httpCtx->response()->addHeader("Set-Cookie", "tbs_selected_lang_id=; Max-Age=0;");
}

bool isValidAppClientId(const std::string& appId)
{
   auto optWebApp = Config::getOption<web::conf::Webapp>("webapp");
   auto acceptedClientAppIds = util::split(optWebApp.webService.acceptedClientAppId, ',');
   
   // find TBSRESTC_DEV, TBSRESTC_TOBASA or other that specified in appsettings.json
   if (util::findPositionInVector(acceptedClientAppIds, appId) == tbs::NOT_FOUND)
      return false;
   else
      return true;
}

} // namespace app
} // namespace tbs