#include <fstream>
#include <chrono>
#include <tobasa/path.h>
#include <tobasa/logger.h>
#include <tobasa/config.h>
#include "tobasaweb/session.h"
#include "tobasaweb/settings_webapp.h"

namespace tbs {
namespace web {

std::mutex Session::_fileMutex;
static std::string sSessionFolder;
static bool sSessionFolderInitialized = false;
static int sSessionExpirationMinutes = 120;

static std::string getSessionFolder()
{
   if (sSessionFolderInitialized)
      return sSessionFolder;
   else
   {
      // default session directory
      sSessionFolder = path::executableDir() + path::SEPARATOR + "appdata" + path::SEPARATOR + "session";

      auto appOption = Config::getOption<web::conf::Webapp>("webapp");
      
      sSessionExpirationMinutes = appOption.webService.sessionExpirationMinutes;

      auto sessPath  = appOption.webService.sessionSavePath;
      if (! sessPath.empty() )
      {
         if (! path::exists(sessPath) )
         {
            if (path::createDir( sessPath ) )
            {
               Logger::logI("[webapp] Session folder created: {}", sessPath);
               sSessionFolder = sessPath;
               sSessionFolderInitialized = true;
               return sSessionFolder;
            }
         }
         else
         {
            sSessionFolder = sessPath;
            sSessionFolderInitialized = true;
            return sSessionFolder;
         }
      }

      if (! path::exists(sSessionFolder) )
      {
         if (path::createDir( sSessionFolder ) )
         {
            Logger::logI("[webapp] Session folder created: {}", sSessionFolder);
            sSessionFolderInitialized = true;
            return sSessionFolder;
         }
      }
      Logger::logI("[webapp] Using default session folder: {}", sSessionFolder);
      sSessionFolderInitialized = true;
      return sSessionFolder;
   }
}


void Session::clearOldSessionFiles()
{
   std::string theFolder = getSessionFolder();
   if (!path::exists(theFolder)) 
      return;

   if (!path::isDirectory(theFolder)) 
      return;

   namespace fs = std::filesystem;
   fs::path folderPath(theFolder);

   if (fs::is_empty(folderPath))
      return;

   // Get the current time
   auto now = std::chrono::system_clock::now();

   // Iterate through the directory and remove files older than 24 hours
   try 
   {
      Logger::logI("[webapp] Clearing old session files in {}", theFolder);
      for (const auto& entry : fs::directory_iterator(folderPath)) 
      {
         //fs::remove_all(entry);
         if (fs::is_regular_file(entry.status()))
         {
               // Convert file time to system_clock time point
               auto fileTime = fs::last_write_time(entry);
               auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                  fileTime - fs::file_time_type::clock::now() + now);

               // Check if file age exceeds session expiration time
               if (now - sctp > std::chrono::minutes(sSessionExpirationMinutes))
               {
                  fs::remove(entry);
                  Logger::logI("[webapp] Deleted old session file: {}", entry.path().string());
               }
         }
      }
   } 
   catch (const fs::filesystem_error& e) 
   {
      Logger::logE("[webapp] Failed clearing old session files: {}", e.what());
   } 
   catch (const std::exception& e) 
   {
      Logger::logE("[webapp] Failed clearing old session files: {}", e.what());
   }
}


SessionPtr Session::create(const std::string& sessionId)
{
   auto ses = std::make_shared<Session>(sessionId);
   ses->save();
   return ses;
}

SessionPtr Session::get(const std::string& sessionId)
{
   auto ses = std::make_shared<Session>(sessionId);
   ses->readStorage();
   return ses;
}

Session::Session(const std::string& sessionId)
{
   _loaded    = false;
   _sessionId = sessionId;

   std::string fileName = Session::COOKIE_NAME + "_" + sessionId;
   _storagePath = getSessionFolder() + path::SEPARATOR +  fileName;
}

Session::~Session() {}

bool Session::destroy(const std::string& sessionId)
{
   if ( sessionId.empty() || sessionId.length() != Session::COOKIE_ID_LENGTH)
   {
      Logger::logD("[webapp] Error occurred while destroying session: invalid session id");
      return false;
   }

   auto ses      = Session(sessionId);
   auto filePath = ses.storagePath();

   if (path::exists(filePath))
   {
      std::lock_guard<std::mutex> lock(_fileMutex);

      if (std::remove(filePath.c_str()) != 0)
      {
         Logger::logE("[webapp] Error occured while deleting session file: {}, error: {}", filePath, std::strerror(errno) );
         return false;
      }
   }

   return true;
}

void Session::readStorage(bool reload)
{
   if (_loaded && !reload)
      return;

   try
   {
      if (! path::exists(_storagePath))
         Logger::logT("[webapp] Session file does not exist: {}",  _storagePath);

      std::ifstream is(_storagePath, std::ifstream::in);
      if (is.good())
      {
         _data = Json::parse(is, /*callback*/ nullptr, /*allow exceptions*/ true, /*ignore_comments*/ true);
         _loaded = true;
      }
   }
   catch (Json::exception& ex)
   {
      Logger::logE("[webapp] Failed to load session file: {}",  ex.what());
   }
   catch (const std::exception& ex)
   {
      Logger::logE("[webapp] Failed to load session file: {}",  ex.what());
   }
}

Json Session::getData(const std::string& key)
{
   readStorage();

   if (_data.contains(key))
      return _data[key];
   else
      return {};
}

void Session::setData(const std::string& key, const Json& value)
{
   readStorage();

   _data[key] = value;
   save();
}

void Session::setAlerts(Json alerts)
{
   // Read data from disk
   readStorage(true);

   _data["alerts"] = alerts;
   save();
}

Json Session::getAlerts()
{
   // Read data from disk
   readStorage(true);

   // get alerts, we want to send to client
   auto alerts = _data["alerts"];
   
   // clear stored alerts
   _data["alerts"] = {};
   save();

   if (alerts.empty())
      return Json::array();
   else
      return alerts;
}

void Session::save()
{
   std::lock_guard<std::mutex> lock(_fileMutex);  // Lock the mutex to prevent race conditions
   
   std::string folder = sessionFolder();

   if (! path::exists( folder ))
   {
      if (path::createDir( folder ) )
         Logger::logI("[webapp] Creating session folder : {}",  folder);
      else
         Logger::logE("[webapp] Failed creating session folder: {}",  folder);
   }

   if (path::exists( folder ))
   {
      // Creates the file if it does not exist and truncates it if it does
      std::ofstream file(_storagePath);
      std::string data = _data.dump();
      file << data;
   }
   else
      Logger::logT("[webapp] Session folder does not exist: {}",  folder);
}

std::string Session::storagePath()
{
   return _storagePath;
}

bool Session::loaded()
{
   return _loaded;
}


// tbs_session=n0cfsk1adfqpd9qnj37b5h4qp1; Path=/; 
// tbs_session=n0cfsk1adfqpd9qnj37b5h4qp1; Path=/; Max-Age=2592000

std::string Session::createDefaultCookie(const std::string& sessionId)
{
   return Session::COOKIE_NAME + "=" + sessionId + "; Path=/;";// +  " Secure; HttpOnly";
}

// Method to generate the cookie string
std::string Cookie::generate() const 
{
   std::ostringstream cookieStream;

   cookieStream << name << "=" << value << "; ";

   cookieStream << "Max-Age=" << maxAge << "; ";

   if (!path.empty()) {
      cookieStream << "Path=" << path << "; ";
   }

   if (httpOnly) {
      cookieStream << "HttpOnly; ";
   }

   if (secure) {
      cookieStream << "Secure; ";
   }

   if (!sameSite.empty()) {
      cookieStream << "SameSite=" << sameSite << "; ";
   }

   return cookieStream.str();
}

std::string Session::sessionFolder()
{
   return getSessionFolder();
}

} // namespace web
} // namespace tbs