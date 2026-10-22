#include <sqlite3.h>
#include <tobasa/logger.h>
#include <tobasa/path.h>

#include "sqlite_session_store.h"

namespace tbs {
namespace web {
namespace {

bool isJsonObject(const Json& value)
{
   return value.is_object();
}

std::string jsonStringFrom(const Json& value)
{
   if (value.is_null())
      return "null";
      
   return value.dump();
}

std::string defaultSqliteDatabasePath()
{
   return path::executableDir() + path::SEPARATOR + "appdata" + path::SEPARATOR + "session_store.db";
}

} // namespace

SqliteSessionStore::SqliteSessionStore(const std::string& dbPath)
   : _dbPath(dbPath.empty() ? defaultSqliteDatabasePath() : dbPath)
{
   std::filesystem::path databaseFile(_dbPath);
   if (!databaseFile.parent_path().empty())
      std::filesystem::create_directories(databaseFile.parent_path());

   initializeDatabase();
}

SqliteSessionStore::~SqliteSessionStore()
{
   shutdown();
}

bool SqliteSessionStore::isOpen() const
{
   return _db != nullptr;
}

bool SqliteSessionStore::initializeDatabase()
{
   if (sqlite3_open_v2(_dbPath.c_str(), reinterpret_cast<sqlite3**>(&_db), SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK)
   {
      Logger::logE("[webapp] Failed to open SQLite session store: {}", _dbPath);
      sqlite3_close(reinterpret_cast<sqlite3*>(_db));
      _db = nullptr;
      return false;
   }

   sqlite3* sqliteDb = reinterpret_cast<sqlite3*>(_db);
   sqlite3_exec(sqliteDb, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
   sqlite3_exec(sqliteDb, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
   sqlite3_exec(sqliteDb, "PRAGMA busy_timeout=5000;", nullptr, nullptr, nullptr);

   const char* schema =
      "CREATE TABLE IF NOT EXISTS tbs_sessions ("
      "session_id TEXT PRIMARY KEY,"
      "payload_json TEXT NOT NULL,"
      "expires_at INTEGER NOT NULL,"
      "updated_at INTEGER NOT NULL"
      ");"
      "CREATE INDEX IF NOT EXISTS idx_tbs_sessions_expires ON tbs_sessions(expires_at);";

   if (sqlite3_exec(sqliteDb, schema, nullptr, nullptr, nullptr) != SQLITE_OK)
   {
      Logger::logE("[webapp] Failed to initialize SQLite session schema: {}", sqlite3_errmsg(sqliteDb));
      shutdown();
      return false;
   }

   return true;
}

SessionStoreResult SqliteSessionStore::load(const std::string& sessionId)
{
   SessionStoreResult result;
   if (!isOpen())
   {
      result.status = SessionStoreStatus::unavailable;
      result.message = "SQLite store is unavailable";
      return result;
   }

   if (!isValidSessionId(sessionId))
   {
      result.status = SessionStoreStatus::invalid_session_id;
      result.message = "Invalid session ID";
      return result;
   }

   sqlite3_stmt* statement = nullptr;
   const char* sql = "SELECT payload_json, expires_at FROM tbs_sessions WHERE session_id = ?;";
   if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(_db), sql, -1, &statement, nullptr) != SQLITE_OK)
   {
      result.status = SessionStoreStatus::io_error;
      result.message = sqlite3_errmsg(reinterpret_cast<sqlite3*>(_db));
      return result;
   }

   sqlite3_bind_text(statement, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);

   const int stepResult = sqlite3_step(statement);
   if (stepResult == SQLITE_ROW)
   {
      const unsigned char* payloadText = sqlite3_column_text(statement, 0);
      const long long expiresAtUtcSeconds = sqlite3_column_int64(statement, 1);
      if (payloadText == nullptr)
      {
         result.status = SessionStoreStatus::invalid_payload;
         result.message = "Stored payload is empty";
         sqlite3_finalize(statement);
         return result;
      }

      try
      {
         result.value = Json::parse(reinterpret_cast<const char*>(payloadText), nullptr, true, true);
      }
      catch (const Json::exception& ex)
      {
         result.status = SessionStoreStatus::serialization_error;
         result.message = ex.what();
         sqlite3_finalize(statement);
         return result;
      }

      const auto now = std::chrono::duration_cast<std::chrono::seconds>(
         std::chrono::system_clock::now().time_since_epoch()).count();
      if (expiresAtUtcSeconds > 0 && expiresAtUtcSeconds <= now)
      {
         result.status = SessionStoreStatus::expired;
         result.message = "Session expired";
      }
      else
      {
         result.status = SessionStoreStatus::ok;
      }
   }
   else if (stepResult == SQLITE_DONE)
   {
      result.status = SessionStoreStatus::not_found;
      result.message = "Session not found";
   }
   else
   {
      result.status = SessionStoreStatus::io_error;
      result.message = sqlite3_errmsg(reinterpret_cast<sqlite3*>(_db));
   }

   sqlite3_finalize(statement);
   return result;
}

bool SqliteSessionStore::save(const std::string& sessionId, const Json& value, long long expiresAtUtcSeconds)
{
   if (!isOpen() || !isValidSessionId(sessionId) || !isJsonObject(value))
      return false;

   Json payload = value;
   if (!payload.contains("expires") || payload["expires"].empty())
      payload["expires"] = expiresAtUtcSeconds;

   const auto now = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();

   sqlite3_stmt* statement = nullptr;
   const char* sql =
      "INSERT INTO tbs_sessions(session_id, payload_json, expires_at, updated_at) "
      "VALUES(?, ?, ?, ?) "
      "ON CONFLICT(session_id) DO UPDATE SET payload_json = excluded.payload_json, "
      "expires_at = excluded.expires_at, updated_at = excluded.updated_at;";

   if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(_db), sql, -1, &statement, nullptr) != SQLITE_OK)
      return false;

   sqlite3_bind_text(statement, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);
   sqlite3_bind_text(statement, 2, jsonStringFrom(payload).c_str(), -1, SQLITE_TRANSIENT);
   sqlite3_bind_int64(statement, 3, expiresAtUtcSeconds);
   sqlite3_bind_int64(statement, 4, now);

   const bool ok = sqlite3_step(statement) == SQLITE_DONE;
   sqlite3_finalize(statement);
   return ok;
}

bool SqliteSessionStore::remove(const std::string& sessionId)
{
   if (!isOpen() || !isValidSessionId(sessionId))
      return false;

   sqlite3_stmt* statement = nullptr;
   const char* sql = "DELETE FROM tbs_sessions WHERE session_id = ?;";
   if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(_db), sql, -1, &statement, nullptr) != SQLITE_OK)
      return false;

   sqlite3_bind_text(statement, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);
   const bool ok = sqlite3_step(statement) == SQLITE_DONE;
   sqlite3_finalize(statement);
   return ok;
}

bool SqliteSessionStore::cleanupExpired(long long nowUtcSeconds)
{
   if (!isOpen())
      return false;

   sqlite3_stmt* statement = nullptr;
   const char* sql = "DELETE FROM tbs_sessions WHERE expires_at > 0 AND expires_at <= ?;";
   if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(_db), sql, -1, &statement, nullptr) != SQLITE_OK)
      return false;

   sqlite3_bind_int64(statement, 1, nowUtcSeconds);
   const bool ok = sqlite3_step(statement) == SQLITE_DONE;
   sqlite3_finalize(statement);
   return ok;
}

void SqliteSessionStore::shutdown()
{
   if (_db != nullptr)
   {
      sqlite3_close(reinterpret_cast<sqlite3*>(_db));
      _db = nullptr;
   }
}

} // namespace web
} // namespace tbs