#pragma once

#include "tobasahttp/settings.h"

namespace tbs {
namespace http {

const size_t  HTTP_COMPRESS_MIN_SIZE            = 1024; // in bytes

/// A fluent style interface for setting http server params.
template <typename DerivedType>
class SettingsServerCommon 
   : public SettingsBase<DerivedType>
{
   using BaseType = SettingsBase<DerivedType>;

protected:
   using Derived  = DerivedType;
   // helpers for fluent API
   Derived& self() & { return static_cast<Derived&>(*this); }
   Derived&& self() && { return static_cast<Derived&&>(*this); }

private:

   std::size_t    _wsMessageMaxSize  { HTTP_WS_MSG_MAX_SIZE_DEFAULT };      // in bytes
   std::size_t    _clientBodyMaxSize { HTTP_CLIENT_BODY_MAX_SIZE_DEFAULT }; // in bytes

   /// Activate HTTP connection rate limiter
   bool           _useRateLimiter            { false };
   /// Maximum number of requests from single IP allowed within the time window.
   int32_t        _rateLimiterMaxRequests    { 10 };
   /// Duration of the time window for counting requests (in milliseconds)
   int32_t        _rateLimiterWindowDuration { 1000 };
   ///  Duration for which an IP is temporarily blocked (in milliseconds)
   int32_t        _rateLimiterBlockDuration  { 30*1000 };
   /// Number of violations allowed before permanently blacklisting an IP.
   int32_t        _rateLimiterMaxViolations  { 3 };

   bool           _compressionEnable         { true };
   int32_t        _compressionMinimalLength  { HTTP_COMPRESS_MIN_SIZE };
   std::string    _compressionEncoding       { "gzip" };

   std::vector<std::string> _compressionMimeTypes {
      "text/plain",
      "text/css",
      "application/json",
      "application/javascript",
      "text/xml",
      "application/xml",
      "image/svg+xml"
   };

public:
   using BaseType::BaseType;

   // ------------------------------------------------
   // WebSocket maximum message 
   // default 8KB, max 1 MB
   Derived& wsMessageMaxSize(std::size_t val) &
   {
      _wsMessageMaxSize = self().clampValue(val, HTTP_WS_MSG_MAX_SIZE_MIN, HTTP_WS_MSG_MAX_SIZE_MAX, HTTP_WS_MSG_MAX_SIZE_DEFAULT);
      return self();
   }
   Derived&& wsMessageMaxSize(std::size_t val) &&
   {
      return std::move(self().wsMessageMaxSize(val));
   }
   [[nodiscard]] std::size_t wsMessageMaxSize() const { return _wsMessageMaxSize; }

   // ------------------------------------------------
   Derived& useRateLimiter(bool value) &
   {
      _useRateLimiter = value;
      return self();
   }
   Derived&& useRateLimiter(bool val) &&
   {
      return std::move(self().useRateLimiter(val));
   }
   [[nodiscard]] bool useRateLimiter() const { return _useRateLimiter; }

   // ------------------------------------------------
   /// Maximum number of requests from single IP allowed within the time window.
   /// valid values 1-200, default 10
   Derived& rateLimiterMaxRequests(int32_t val) &
   {
      _rateLimiterMaxRequests = self().clampValue(val, (int32_t)1 , (int32_t)200, (int32_t)10);
      return self();
   }
   Derived&& rateLimiterMaxRequests(int32_t val) &&
   {
      return std::move(self().rateLimiterMaxRequests(val));
   }
   [[nodiscard]] int32_t rateLimiterMaxRequests() const { return _rateLimiterMaxRequests; }

   // ------------------------------------------------
   /// Duration of the time window for counting requests (in milliseconds)
   /// valid values 1-3600000ms (1hour), default 1000ms
   Derived& rateLimiterWindowDuration(int32_t val) &
   {
      _rateLimiterWindowDuration = self().clampValue(val, (int32_t)1 , (int32_t)(60*60*1000), (int32_t)1000);
      return self();
   }
   Derived&& rateLimiterWindowDuration(int32_t val) &&
   {
      return std::move(self().rateLimiterWindowDuration(val));
   }
   [[nodiscard]] int32_t rateLimiterWindowDuration() const { return _rateLimiterWindowDuration; }

   // ------------------------------------------------
   ///  Duration for which an IP is temporarily blocked (in milliseconds)
   /// valid values 1-86400000(24 hours), default 30s
   Derived& rateLimiterBlockDuration(int32_t val) &
   {
      _rateLimiterBlockDuration = self().clampValue(val, (int32_t)1, (int32_t)(60*60*24*1000), (int32_t)(30*1000) );
      return self();
   }
   Derived&& rateLimiterBlockDuration(int32_t val) &&
   {
      return std::move(self().rateLimiterBlockDuration(val));
   }
   [[nodiscard]] int32_t rateLimiterBlockDuration() const { return _rateLimiterBlockDuration; }

   // ------------------------------------------------
   /// Number of violations allowed before permanently blacklisting an IP. 
   // valid values 1-1000, default 3
   Derived& rateLimiterMaxViolations(int32_t val) &
   {
      _rateLimiterMaxViolations = self().clampValue(val, (int32_t)1, (int32_t)1000, (int32_t)3);
      return self();
   }
   Derived&& rateLimiterMaxViolations(int32_t val) &&
   {
      return std::move(self().rateLimiterMaxViolations(val));
   }
   [[nodiscard]] int32_t rateLimiterMaxViolations() const { return _rateLimiterMaxViolations; }

   // ------------------------------------------------
   // Client body maximum size
   // default unlimited, max 50 MB
   Derived& clientBodyMaxSize(std::size_t val) &
   {
      if (val <= 0)
         _clientBodyMaxSize = HTTP_CLIENT_BODY_MAX_SIZE_DEFAULT;
      else
         _clientBodyMaxSize = val <= HTTP_CLIENT_BODY_MAX_SIZE_MAX ? val : HTTP_CLIENT_BODY_MAX_SIZE_MAX;
      
      return self();
   }
   Derived&& clientBodyMaxSize(std::string val) &&
   {
      return std::move(self().clientBodyMaxSize(val));
   }
   [[nodiscard]] std::size_t clientBodyMaxSize() const { return _clientBodyMaxSize; }

   // ------------------------------------------------
   Derived& useCompression(bool val) &
   {
      _compressionEnable = val;
      return self();
   }
   Derived&& useCompression(bool val) &&
   {
      return std::move(self().useCompression(val));
   }
   [[nodiscard]] bool useCompression() const { return _compressionEnable; }

   // ------------------------------------------------
   // default 1024
   Derived& compressionMinimalLength(int32_t val) &
   {
      if (val >= HTTP_COMPRESS_MIN_SIZE)
         _compressionMinimalLength = val;
      else
         _compressionMinimalLength = HTTP_COMPRESS_MIN_SIZE;

      return self();
   }
   Derived&& compressionMinimalLength(int32_t val) &&
   {
      return std::move(self().compressionMinimalLength(val));
   }
   [[nodiscard]] int32_t compressionMinimalLength() const { return _compressionMinimalLength; }

   // ------------------------------------------------
   Derived& compressionEncoding(std::string encoding) &
   {
      _compressionEncoding = std::move(encoding);
      return self();
   }
   Derived&& compressionEncoding(std::string val) &&
   {
      return std::move(self().compressionEncoding(val));
   }
   [[nodiscard]] std::string compressionEncoding() const { return _compressionEncoding; }

   // ------------------------------------------------
   Derived& compressionMimeTypes(std::string types) &
   {
      _compressionMimeTypes = util::split(std::move(types), ' ');
      return self();
   }
   Derived&& compressionMimeTypes(std::string val) &&
   {
      return std::move(self().compressionMimeTypes(val));
   }
   [[nodiscard]] std::vector<std::string> compressionMimeTypes() const { return _compressionMimeTypes; }

};


// HTTP Server settings
class Settings final
   : public SettingsServerCommon<Settings>
{
};


/** @}*/

} // namespace http
} // namespace tbs