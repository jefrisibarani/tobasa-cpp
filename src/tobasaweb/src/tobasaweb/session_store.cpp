#include "tobasaweb/session_store.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>


#include <tobasa/logger.h>
#include <tobasa/path.h>

namespace tbs {
namespace web {
namespace {

std::string defaultSessionRootPath()
{
   return path::executableDir() + path::SEPARATOR + "appdata" + path::SEPARATOR + "session";
}

std::string normalizeBackendName(const std::string& value)
{
   std::string normalized = value;
   std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch)
      {
         return static_cast<char>(std::tolower(ch));
      });
   return normalized;
}

bool isJsonObject(const Json& value)
{
   return value.is_object();
}

} // namespace

bool isValidSessionId(const std::string& sessionId)
{
   return !sessionId.empty() && sessionId.size() == 36;
}


FileSessionStore::FileSessionStore(const std::string& sessionRootPath)
   : _sessionRootPath(sessionRootPath.empty() ? defaultSessionRootPath() : sessionRootPath)
{
   std::filesystem::create_directories(_sessionRootPath);
}

FileSessionStore::~FileSessionStore() = default;

std::string FileSessionStore::sessionRootPath() const
{
   return _sessionRootPath;
}

std::string FileSessionStore::sessionPathFor(const std::string& sessionId) const
{
   return _sessionRootPath + path::SEPARATOR + "tbs_session_" + sessionId;
}

SessionStoreResult FileSessionStore::load(const std::string& sessionId)
{
   SessionStoreResult result;
   result.status = SessionStoreStatus::ok;

   if (!isValidSessionId(sessionId))
   {
      result.status = SessionStoreStatus::invalid_session_id;
      result.message = "Invalid session ID";
      return result;
   }

   const auto path = sessionPathFor(sessionId);
   if (!std::filesystem::exists(path))
   {
      result.status = SessionStoreStatus::not_found;
      result.message = "Session not found";
      return result;
   }

   std::ifstream input(path, std::ios::in | std::ios::binary);
   if (!input.good())
   {
      result.status = SessionStoreStatus::io_error;
      result.message = "Unable to open session file";
      return result;
   }

   try
   {
      const std::string payload((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>() );
      result.value = Json::parse(payload, nullptr, true, true);
   }
   catch (const Json::exception& ex)
   {
      result.status = SessionStoreStatus::serialization_error;
      result.message = ex.what();
      return result;
   }
   catch (const std::exception& ex)
   {
      result.status = SessionStoreStatus::serialization_error;
      result.message = ex.what();
      return result;
   }

   if (!isJsonObject(result.value))
   {
      result.status = SessionStoreStatus::invalid_payload;
      result.message = "Session payload is not a JSON object";
      return result;
   }

   if (result.value.contains("expires") && !result.value["expires"].empty())
   {
      const auto expiresValue = result.value["expires"];
      if (expiresValue.is_number_integer() || expiresValue.is_number_unsigned())
      {
         const long long expiresAtUtcSeconds = expiresValue.get<long long>();
         const auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
         if (expiresAtUtcSeconds > 0 && expiresAtUtcSeconds <= now)
         {
            result.status = SessionStoreStatus::expired;
            result.message = "Session expired";
            return result;
         }
      }
   }

   return result;
}

bool FileSessionStore::save(const std::string& sessionId, const Json& value, long long expiresAtUtcSeconds)
{
   if (!isValidSessionId(sessionId))
      return false;

   if (!isJsonObject(value))
      return false;

   Json payload = value;
   if (!payload.contains("expires") || payload["expires"].empty())
      payload["expires"] = expiresAtUtcSeconds;

   std::filesystem::create_directories(_sessionRootPath);
   const auto filePath = sessionPathFor(sessionId);
   std::ofstream output(filePath, std::ios::out | std::ios::trunc | std::ios::binary);
   if (!output.good())
      return false;

   output << payload.dump();
   return output.good();
}

bool FileSessionStore::remove(const std::string& sessionId)
{
   if (!isValidSessionId(sessionId))
      return false;

   const auto filePath = sessionPathFor(sessionId);
   if (!std::filesystem::exists(filePath))
      return true;

   std::error_code error;
   if (!std::filesystem::remove(filePath, error))
   {
      Logger::logE("[webapp] Failed to remove file-based session: {} {}", filePath, error.message());
      return false;
   }
   return true;
}

bool FileSessionStore::cleanupExpired(long long nowUtcSeconds)
{
   if (!std::filesystem::exists(_sessionRootPath))
      return true;

   bool allGood = true;
   for (const auto& entry : std::filesystem::directory_iterator(_sessionRootPath))
   {
      if (!entry.is_regular_file())
         continue;

      const auto fileName = entry.path().filename().string();
      if (fileName.rfind("tbs_session_", 0) != 0)
         continue;

      const auto sessionId = fileName.substr(std::string("tbs_session_").size());
      if (!isValidSessionId(sessionId))
         continue;

      const auto result = load(sessionId);
      if (result.status == SessionStoreStatus::expired || result.status == SessionStoreStatus::not_found)
      {
         if (!remove(sessionId))
            allGood = false;
      }
   }

   return allGood && (nowUtcSeconds >= 0);
}

void FileSessionStore::shutdown()
{
   // no-op for the file backend.
}


// std::unique_ptr<ISessionStore> makeSessionStore(
//    const std::string& backendName,
//    const std::string& sessionRootPath,
//    const std::string& databasePath)
// {
//    const std::string normalized = normalizeBackendName(backendName);
//    if (normalized == "sqlite" || normalized == "sqlite3")
//       return std::make_unique<SqliteSessionStore>(databasePath.empty() ? defaultSqliteDatabasePath() : databasePath);

//    return std::make_unique<FileSessionStore>(sessionRootPath.empty() ? defaultSessionRootPath() : sessionRootPath);
// }

} // namespace web
} // namespace tbs
