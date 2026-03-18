#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <mutex>
#include <tobasa/json.h>

namespace tbs {
namespace web {

class Session;

using SessionPtr = std::shared_ptr<Session>;

class Session
{
public:
   static inline const std::string COOKIE_NAME = "tbs_session";
   static inline const int COOKIE_ID_LENGTH = 36;

   Session(const std::string& sessionId);
   ~Session();

   void setAlerts(Json alerts);
   Json getAlerts();
   void save();

   Json getData(const std::string& key);
   void setData(const std::string& key, const Json& value);
   
   /// Full path of the session file
   std::string storagePath();
   
   /// Check wether the session file successfuly loaded from storage
   bool loaded();

   static SessionPtr create(const std::string& sessionId);
   static SessionPtr get(const std::string& sessionId);
   static bool destroy(const std::string& sessionId);
   static std::string createDefaultCookie(const std::string& sessionId);
   static void clearOldSessionFiles();
   static std::string sessionFolder();

private:
   // Read data, if reload is true, reload data from disk
   void readStorage(bool reload=false);

   bool        _loaded = false;
   std::string _sessionId;
   std::string _storagePath;
   Json        _data;
   static std::mutex _fileMutex;
};

/*
   Expires Attribute: Specifies the expiration date and time of the cookie. Without this attribute, the cookie is a session cookie.
   Session Cookies: Last only for the duration of the browser session and are deleted when the browser is closed.
*/

class Cookie
{
public:
   std::string name;
   std::string value;
   bool httpOnly;
   bool secure;
   std::string sameSite;
   int maxAge;       // in seconds
   std::string path; // Cookie path

   // Generate the cookie string
   std::string generate() const; 
};


} // namespace web
} // namespace tbs