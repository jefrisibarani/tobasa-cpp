#include <sstream>
#include <ctime>
#include "tobasahttp/cookie_response.h"

namespace tbs {
namespace http {


ResponseCookiePtr ResponseCookie::create(const std::string& name, const std::string& value, int32_t maxAge)
{
   auto cookie = std::make_shared<ResponseCookie>(name, value, maxAge);
   return cookie;
}

ResponseCookiePtr ResponseCookie::remove(const std::string& name)
{
   auto cookie = std::make_shared<ResponseCookie>(name, true);
   return cookie;
}

// Helper to generate a GMT time string for Expires attribute
std::string ResponseCookie::generateExpires(int32_t secondsFromNow) const 
{
   std::time_t now = std::time(nullptr) + secondsFromNow;
   char buffer[30];
   std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&now));
   return std::string(buffer);
}

// Constructor
ResponseCookie::ResponseCookie(const std::string& name, const std::string& value, int32_t maxAge)
   : _name {name}, _value{value}, _maxAge{maxAge}, _reset {false}
{
   _expires = generateExpires(maxAge);
}

ResponseCookie::ResponseCookie(const std::string& name, bool reset)
   : _name {name}, _reset {reset} 
{
}

// Getters and chainable setters
const std::string& ResponseCookie::name() const { return _name; }

ResponseCookie& ResponseCookie::name(const std::string& val) 
{
   _name = val;
   return *this;
}

const std::string& ResponseCookie::value() const { return _value; }

ResponseCookie& ResponseCookie::value(const std::string& val) 
{
   _value = val;
   return *this;
}

const std::string& ResponseCookie::path() const { return _path; }

ResponseCookie& ResponseCookie::path(const std::string& val) 
{
   _path = val;
   return *this;
}

const std::string& ResponseCookie::domain() const { return _domain; }

ResponseCookie& ResponseCookie::domain(const std::string& val) 
{
   _domain = val;
   return *this;
}

bool ResponseCookie::httpOnly() const { return _httpOnly; }

ResponseCookie& ResponseCookie::httpOnly(bool val) 
{
   _httpOnly = val;
   return *this;
}

bool ResponseCookie::secure() const  { return _secure; }

ResponseCookie& ResponseCookie::secure(bool val) 
{
   _secure = val;
   return *this;
}

const std::string& ResponseCookie::sameSite() const { return _sameSite; }

ResponseCookie& ResponseCookie::sameSite(const std::string& val) 
{
   _sameSite = val;
   return *this;
}

int32_t ResponseCookie::maxAge() const { return _maxAge; }

ResponseCookie& ResponseCookie::maxAge(int32_t val) 
{
   _maxAge = val;
   _expires = generateExpires(val);
   return *this;
}

const std::string& ResponseCookie::expires() const { return _expires; }

ResponseCookie& ResponseCookie::expires(const std::string& val) 
{
   _expires = val;
   return *this;
}

const std::string& ResponseCookie::priority() const { return _priority; }

ResponseCookie& ResponseCookie::priority(const std::string& val) 
{
   _priority = val;
   return *this;
}

bool ResponseCookie::sameParty() const { return _sameParty; }

ResponseCookie& ResponseCookie::sameParty(bool val) 
{
   _sameParty = val;
   return *this;
}

bool ResponseCookie::partitioned() const { return _partitioned; }

ResponseCookie& ResponseCookie::partitioned(bool val) 
{
   _partitioned = val;
   return *this;
}

// toString method
std::string ResponseCookie::toString(bool reset) const 
{
   std::ostringstream oss;

   oss << _name << "=";
   
   if (_reset || reset) 
   {
      oss << "; Max-Age=0;" ;
   } 
   else 
   {
      oss << _value;
      if (!_path.empty())     oss << "; Path="     << _path;
      if (!_domain.empty())   oss << "; Domain="   << _domain;
      if (_httpOnly)          oss << "; HttpOnly";
      if (_secure)            oss << "; Secure";
      if (!_sameSite.empty()) oss << "; SameSite=" << _sameSite;
      if (_maxAge > 0)        oss << "; Max-Age="  << _maxAge;
      if (!_expires.empty())  oss << "; Expires="  << _expires;
      if (!_priority.empty()) oss << "; Priority=" << _priority;
      if (_sameParty)         oss << "; SameParty";
      if (_partitioned)       oss << "; Partitioned";
   }

   return oss.str();
}

}
}