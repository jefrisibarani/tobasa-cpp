#include <tobasa/logger.h>
#include <tobasa/crypt.h>
#include <tobasa/datetime.h>
#include <tobasa/util_date.h>
#include <tobasa/json.h>
#include <tobasahttp/multipart_body_reader.h>
#include <tobasaweb/json_result.h>
#include <tobasasql/exception.h>
#include "test_controller.h"

namespace tbs {
namespace test {

using namespace http;

//! Handle request to /test/crypto
http::ResultPtr TestController::onCrypto(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto request = httpCtx->request();

   if (!util::startsWith(httpCtx->request()->contentType(), "application/json"))
      return jsonResult(StatusCode::BAD_REQUEST);

   try
   {
      std::string cryptSalt, clearPasswordSrc, encryptedPasswordSrc, encryptedPassword, decryptedPassword, textToMd5;
      std::string encryptedPasswordX, decryptedPasswordX;

      auto body            = request->content();
      auto jsonDto         = Json::parse(body);

      cryptSalt            = jsonDto["crypt_salt"];
      clearPasswordSrc     = jsonDto["clear_password_src"];
      encryptedPasswordSrc = jsonDto["encrypted_password_src"];
      textToMd5            = jsonDto["text_to_md5"];
      textToMd5            = crypt::hashMD5(textToMd5);
      encryptedPassword    = crypt::passwordEncrypt(clearPasswordSrc,     cryptSalt);
      decryptedPassword    = crypt::passwordDecrypt(encryptedPasswordSrc, cryptSalt);

      Json jsonResultBody;
      jsonResultBody["text_to_md5_hash"]       = textToMd5;
      jsonResultBody["clear_password_src"]     = clearPasswordSrc;
      jsonResultBody["encrypted_password"]     = encryptedPassword;
      jsonResultBody["encrypted_password_src"] = encryptedPasswordSrc;
      jsonResultBody["decrypred_password"]     = decryptedPassword;

      return jsonResult(jsonResultBody);
   }
   catch (const SqlException &ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return jsonResult(StatusCode::INTERNAL_SERVER_ERROR, "SQL server error");
   }
   catch (const tbs::AppException &ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return jsonResult(StatusCode::INTERNAL_SERVER_ERROR, ex.what());
   }
   catch (const Json::exception &ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return jsonResult(StatusCode::BAD_REQUEST, "JSON data error: " + cleanJsonException(ex));
   }
   catch (const std::exception &ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return jsonResult(StatusCode::INTERNAL_SERVER_ERROR);
   }
}


//! Handle POST request to /test/datetime
http::ResultPtr TestController::onDateTime(const web::RouteArgument& arg)
{
   // simulate long processing
   //std::this_thread::sleep_for(std::chrono::milliseconds(15000));

   auto httpCtx = arg.httpContext();
   auto request = httpCtx->request();

   if (!util::startsWith(httpCtx->request()->contentType(), "application/json"))
      return jsonResult(StatusCode::BAD_REQUEST);

   Json res;

   // init DateTime with current time
   DateTime dt0;
   auto dt0_isodate  = dt0.isoDateString();
   auto dt0_isodt    = dt0.isoDateTimeString(true);
   auto dt0_isodtUtc = dt0.isoDateTimeStringUTC(true);
   auto dt0_unixTime = dt0.toUnixTimeMiliSeconds();
   res["dt0"]["isoDateString"]            = dt0_isodate;
   res["dt0"]["isoDateTimeString"]        = dt0_isodt;
   res["dt0"]["isoDateTimeStringUTC"]     = dt0_isodtUtc;
   res["dt0"]["unixtime_miliseconds"]     = dt0_unixTime;


   // create new datetime object, then parse dt0's ISO date time string
   DateTime dt1;
   dt1.parse(dt0_isodt);
   auto dt1_isodate  = dt1.isoDateString();
   auto dt1_isodt    = dt1.isoDateTimeString(true);
   auto dt1_isodtUtc = dt1.isoDateTimeStringUTC(true);
   auto dt1_unixTime = dt1.toUnixTimeMiliSeconds();
   res["dt1"]["isoDateString"]            = dt1_isodate;
   res["dt1"]["isoDateTimeString"]        = dt1_isodt;
   res["dt1"]["isoDateTimeStringUTC"]     = dt1_isodtUtc;
   res["dt1"]["unixtime_miliseconds"]     = dt1_unixTime;


   // create new datetime object drom dt0's time point
   DateTime dt2(dt0.timePoint());
   auto dt2_isodate  = dt2.isoDateString();
   auto dt2_isodt    = dt2.isoDateTimeString(true);
   auto dt2_isodtUtc = dt2.isoDateTimeStringUTC(true);
   auto dt2_unixTime = dt2.toUnixTimeMiliSeconds();
   res["dt2"]["isoDateString"]            = dt2_isodate;
   res["dt2"]["isoDateTimeString"]        = dt2_isodt;
   res["dt2"]["isoDateTimeStringUTC"]     = dt2_isodtUtc;
   res["dt2"]["unixtime_miliseconds"]     = dt2_unixTime;

   // Add one year to dt2
   dt2.timePoint() += tbsdate::years{ 2 };
   dt2_isodate  = dt2.isoDateString();
   dt2_isodt    = dt2.isoDateTimeString(true);
   dt2_isodtUtc = dt2.isoDateTimeStringUTC(true);
   dt2_unixTime = dt2.toUnixTimeMiliSeconds();
   res["dt2_a"]["isoDateString"]            = dt2_isodate;
   res["dt2_a"]["isoDateTimeString"]        = dt2_isodt;
   res["dt2_a"]["isoDateTimeStringUTC"]     = dt2_isodtUtc;
   res["dt2_a"]["unixtime_miliseconds"]     = dt2_unixTime;

   // simple test
   auto currentTime = floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
   auto nextYear    = currentTime + tbsdate::years{ 1 };
   auto currentTimeStr = util::formatDate("{:%Y-%m-%d %H:%M:%S}", currentTime);
   auto nextYearStr    = util::formatDate("{:%Y-%m-%d %H:%M:%S}", nextYear);
   res["_currentTimeStr"] = currentTimeStr;
   res["_nextYearStr"]    = nextYearStr;

   auto currentTimeStr2 = util::formatDateNow("{:%Y-%m-%d %H:%M:%S}");
   res["_currentTimeStr2"] = currentTimeStr2;

#if 0
   // Note: Working sample
   // DateTime from time point
   int sessExpire    = 2; // in minutes
   const auto nowUtc = std::chrono::system_clock::now();
   auto expireTimeTp = nowUtc + std::chrono::minutes{ sessExpire };
   auto expiredDt    = DateTime(expireTimeTp);
#endif

   return jsonResult(res);
}


//! Handle POST request to /upload
http::ResultPtr TestController::onUpload(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto request = httpCtx->request();

   auto htmlResult = [](const std::string& dat={}) {
      std::string content = tbsfmt::format(
         "<html><head><title>Tobasa Web Server</title></head>"
         "<body><h1>Welcome {} </h1></body></html>", dat);
      auto result  = http::makeResult();
      result->contentType("text/html");
      result->content(std::move(content));
      return std::move(result);
   };

   if (request->hasMultipartBody())
   {
      auto body = request->multipartBody();
      auto part  = body->find("profileImage");
      if (part && part->isFile)
      {
         //return htmlResult(part->fileName);
         return http::fileResult(part->location);
      }
   }

   return htmlResult();
}

} // namespace test
} // namespace tbs
