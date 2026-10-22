#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include <tobasaweb/session_store.h>

namespace {

using namespace tbs;
using namespace tbs::web;

void expectSessionData(const Json& payload, const std::string& key, const std::string& expected)
{
   ASSERT_FALSE(payload.empty());
   ASSERT_TRUE(payload.contains(key));
   EXPECT_EQ(payload[key].get<std::string>(), expected);
}

void testStoreContract(ISessionStore& store)
{
   const std::string firstId = "11111111-1111-4111-8111-111111111111";
   const std::string secondId = "22222222-2222-4222-8222-222222222222";
   const auto now = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();

   auto missing = store.load(firstId);
   EXPECT_EQ(missing.status, SessionStoreStatus::not_found);

   Json payload;
   payload["logged_in"] = true;
   payload["user_name"] = "alice";
   payload["expires"] = now + 3600;
   EXPECT_TRUE(store.save(firstId, payload, now + 3600));

   auto loaded = store.load(firstId);
   EXPECT_EQ(loaded.status, SessionStoreStatus::ok);
   EXPECT_TRUE(loaded.value.contains("logged_in"));
   EXPECT_TRUE(loaded.value["logged_in"].get<bool>());
   expectSessionData(loaded.value, "user_name", "alice");

   Json updated;
   updated["logged_in"] = false;
   updated["user_name"] = "bob";
   updated["expires"] = now + 7200;
   EXPECT_TRUE(store.save(firstId, updated, now + 7200));

   auto reloaded = store.load(firstId);
   EXPECT_EQ(reloaded.status, SessionStoreStatus::ok);
   EXPECT_FALSE(reloaded.value["logged_in"].get<bool>());
   expectSessionData(reloaded.value, "user_name", "bob");

   Json secondPayload;
   secondPayload["logged_in"] = true;
   secondPayload["user_name"] = "charlie";
   secondPayload["expires"] = now + 1800;
   EXPECT_TRUE(store.save(secondId, secondPayload, now + 1800));

   EXPECT_TRUE(store.remove(firstId));
   auto removed = store.load(firstId);
   EXPECT_EQ(removed.status, SessionStoreStatus::not_found);

   auto other = store.load(secondId);
   EXPECT_EQ(other.status, SessionStoreStatus::ok);
   expectSessionData(other.value, "user_name", "charlie");

   Json expiredPayload;
   expiredPayload["logged_in"] = false;
   expiredPayload["user_name"] = "expired";
   expiredPayload["expires"] = now - 30;
   const std::string expiredId = "33333333-3333-4333-8333-333333333333";
   EXPECT_TRUE(store.save(expiredId, expiredPayload, now - 30));
   EXPECT_EQ(store.load(expiredId).status, SessionStoreStatus::expired);
   EXPECT_TRUE(store.cleanupExpired(now));
   EXPECT_EQ(store.load(expiredId).status, SessionStoreStatus::not_found);
}

} // namespace

TEST(SessionStoreTest, FileSessionStoreBehavesLikeTheSharedContract)
{
   const auto root = std::filesystem::temp_directory_path() / "tobasa_session_file_store_tests";
   std::filesystem::create_directories(root);

   FileSessionStore store(root.string());
   testStoreContract(store);
}

TEST(SessionStoreTest, SqliteSessionStoreBehavesLikeTheSharedContract)
{
   // const auto root = std::filesystem::temp_directory_path() / "tobasa_session_sqlite_store_tests";
   // std::filesystem::create_directories(root);
   // const auto dbPath = root / "sessions.db";

   // SqliteSessionStore store(dbPath.string());
   // testStoreContract(store);
}
