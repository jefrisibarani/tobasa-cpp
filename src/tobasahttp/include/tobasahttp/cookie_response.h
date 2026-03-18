#pragma once
#include <memory>
#include <string>

namespace tbs {
namespace http {

class ResponseCookie;
using ResponseCookiePtr = std::shared_ptr<ResponseCookie>;

/**
 * ResponseCookie, HTTP Response cookie
 * Note:
 * Never call ResponseCookie("tbs_session","12345678"),
 * because second constructor will be called instead of first one.
 */
class ResponseCookie 
{
private:
   std::string _name;
   std::string _value;
   std::string _path          = "/";
   std::string _domain;
   bool        _httpOnly      = false;
   bool        _secure        = false;
   std::string _sameSite      = "Lax";    // Default to "Lax"
   int         _maxAge        = 3600;     // Default to 3600 seconds
   std::string _expires;
   std::string _priority      = "Medium"; // Default to "Medium"
   bool        _sameParty     = false;
   bool        _partitioned   = false;

   bool        _reset         = false;
   std::string generateExpires(int secondsFromNow) const;

public:
   explicit ResponseCookie(const std::string& name, const std::string& value, int maxAge);
   explicit ResponseCookie(const std::string& name, bool reset);

   static ResponseCookiePtr create(const std::string& name, const std::string& value, int maxAge);
   static ResponseCookiePtr remove(const std::string& name);

   const std::string& name() const;
   ResponseCookie& name(const std::string& val);

   const std::string& value() const;
   ResponseCookie& value(const std::string& val);

   const std::string& path() const;
   ResponseCookie& path(const std::string& val);

   const std::string& domain() const;
   ResponseCookie& domain(const std::string& val);

   bool httpOnly() const;
   ResponseCookie& httpOnly(bool val);

   bool secure() const;
   ResponseCookie& secure(bool val);

   const std::string& sameSite() const;
   ResponseCookie& sameSite(const std::string& val);

   int maxAge() const;
   ResponseCookie& maxAge(int val);

   const std::string& expires() const;
   ResponseCookie& expires(const std::string& val);

   const std::string& priority() const;
   ResponseCookie& priority(const std::string& val);

   bool sameParty() const;
   ResponseCookie& sameParty(bool val);

   bool partitioned() const;
   ResponseCookie& partitioned(bool val);

   std::string toString(bool reset = false) const;
};;


} // namespace http
} // namespace tbs