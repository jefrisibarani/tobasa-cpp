#include <gtest/gtest.h>

#include <atomic>
#include <future>
#include <optional>
#include <string>
#include <thread>

#include "tobasa/task_manager.h"

namespace {

TEST(TaskManagerTest, RejectsZeroWorkerCount)
{
   EXPECT_THROW(([] { tbs::TaskManager manager(0); })(), std::invalid_argument);
}

TEST(TaskManagerTest, SnapshotLookupAndDtoIdsStayConsistent)
{
   tbs::TaskManager manager(1);
   std::promise<void> started;
   std::promise<void> done;
   int taskId = -1;

   taskId = manager.addTask(
      "snapshot",
      "info",
      [&]() -> tbs::TaskResult {
         started.set_value();
         std::this_thread::sleep_for(std::chrono::milliseconds(50));
         return tbs::TaskResult::create(tbs::OperationStatus::SUCCESS, "ok");
      },
      [&](const tbs::TaskMetadata& metadata) {
         EXPECT_EQ(metadata.id, taskId);
         done.set_value();
      });

   EXPECT_TRUE(started.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready);
   auto snapshot = manager.getTaskById(taskId);
   ASSERT_TRUE(snapshot.has_value());
   EXPECT_EQ(snapshot->id, taskId);
   EXPECT_EQ(snapshot->name, "snapshot");

   EXPECT_TRUE(done.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready);
   const auto completedSnapshot = manager.getTaskById(taskId);
   ASSERT_TRUE(completedSnapshot.has_value());
   EXPECT_EQ(completedSnapshot->status, "Completed");
   EXPECT_EQ(tbs::TaskMetadataDto::fromTaskMetadata(*completedSnapshot).taskId, taskId);
}

TEST(TaskManagerTest, ReadinessMeansTaskBodyFinishedBeforeCompletionCallback)
{
   tbs::TaskManager manager(1);
   std::promise<void> taskStarted;
   std::promise<void> continueTask;
   std::promise<void> callbackEntered;
   int taskId = -1;

   taskId = manager.addTask(
      "ready",
      "info",
      [&]() -> tbs::TaskResult {
         taskStarted.set_value();
         EXPECT_TRUE(continueTask.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready);
         return tbs::TaskResult::create(tbs::OperationStatus::SUCCESS, "done");
      },
      [&](const tbs::TaskMetadata& metadata) {
         callbackEntered.set_value();
         EXPECT_TRUE(manager.isTaskReady(taskId));
      });

   EXPECT_TRUE(taskStarted.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready);
   EXPECT_FALSE(manager.isTaskReady(taskId));
   continueTask.set_value();
   EXPECT_TRUE(callbackEntered.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready);
   EXPECT_TRUE(manager.isTaskReady(taskId));
}

TEST(TaskManagerTest, TaskAndCompletionExceptionsAreIsolated)
{
   tbs::TaskManager manager(1);
   std::promise<void> callbackFinished;
   std::promise<void> taskStarted;
   int taskId = -1;

   taskId = manager.addTask(
      "exceptions",
      "info",
      [&]() -> tbs::TaskResult {
         taskStarted.set_value();
         throw std::runtime_error("task failure");
      },
      [&](const tbs::TaskMetadata& metadata) {
         EXPECT_EQ(metadata.result.code, static_cast<int>(tbs::OperationStatus::FAILURE));
         EXPECT_EQ(metadata.result.message, "task failure");
         callbackFinished.set_value();
      });

   EXPECT_TRUE(taskStarted.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready);
   EXPECT_TRUE(callbackFinished.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready);
   auto snapshot = manager.getTaskById(taskId);
   ASSERT_TRUE(snapshot.has_value());
   EXPECT_EQ(snapshot->status, "Completed");
   EXPECT_EQ(snapshot->result.code, static_cast<int>(tbs::OperationStatus::FAILURE));

   std::promise<void> callbackFailureReached;
   const auto callbackTaskId = manager.addTask(
      "callback failure",
      "info",
      []() -> tbs::TaskResult {
         return tbs::TaskResult::create(tbs::OperationStatus::SUCCESS, "ok");
      },
      [&](const tbs::TaskMetadata& metadata) {
         callbackFailureReached.set_value();
         throw std::runtime_error("callback failure");
      });

   EXPECT_TRUE(callbackFailureReached.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready);
   EXPECT_TRUE(manager.isTaskReady(callbackTaskId));
}

TEST(TaskManagerTest, StopDrainsAndRejectsNewWork)
{
   tbs::TaskManager manager(1);
   std::atomic<int> executed{0};
   std::promise<void> taskStarted;
   std::promise<void> done;

   const auto firstId = manager.addTask(
      "first",
      "info",
      [&]() -> tbs::TaskResult {
         taskStarted.set_value();
         ++executed;
         done.set_value();
         return tbs::TaskResult::create(tbs::OperationStatus::SUCCESS, "done");
      },
      nullptr);

   EXPECT_TRUE(taskStarted.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready);
   EXPECT_TRUE(done.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready);
   EXPECT_EQ(executed.load(), 1);
   EXPECT_TRUE(manager.isTaskReady(firstId));

   manager.stop();
   EXPECT_THROW(manager.addTask("later", "info", []() -> tbs::TaskResult {
      return tbs::TaskResult::create(tbs::OperationStatus::SUCCESS, "later");
   }), std::logic_error);
}

TEST(TaskManagerTest, StopFromTaskIsRejectedAndManagerRemainsRunning)
{
   tbs::TaskManager manager(1);
   std::promise<void> taskCompleted;
   const auto taskId = manager.addTask(
      "task-stop",
      "info",
      [&]() -> tbs::TaskResult {
         manager.stop();
         return tbs::TaskResult::create(tbs::OperationStatus::SUCCESS, "unreachable");
      },
      [&](const tbs::TaskMetadata& metadata) {
         EXPECT_EQ(metadata.result.code, static_cast<int>(tbs::OperationStatus::FAILURE));
         EXPECT_EQ(metadata.result.message, "TaskManager workers cannot stop their manager");
         taskCompleted.set_value();
      });

   EXPECT_TRUE(taskCompleted.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready);
   EXPECT_TRUE(manager.isTaskReady(taskId));

   std::promise<void> followUpCompleted;
   manager.addTask(
      "follow-up",
      "info",
      []() -> tbs::TaskResult {
         return tbs::TaskResult::create(tbs::OperationStatus::SUCCESS, "ok");
      },
      [&](const tbs::TaskMetadata&) { followUpCompleted.set_value(); });
   EXPECT_TRUE(followUpCompleted.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready);
}

TEST(TaskManagerTest, StopFromCallbackIsRejectedAndManagerRemainsRunning)
{
   tbs::TaskManager manager(1);
   std::promise<void> callbackStopRejected;
   int taskId = -1;

   taskId = manager.addTask(
      "self-stop",
      "info",
      []() -> tbs::TaskResult {
         return tbs::TaskResult::create(tbs::OperationStatus::SUCCESS, "ok");
      },
      [&](const tbs::TaskMetadata& metadata) {
         try {
            manager.stop();
         }
         catch (const std::logic_error&) {
            callbackStopRejected.set_value();
         }
      });

   EXPECT_TRUE(callbackStopRejected.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready);
   EXPECT_TRUE(manager.isTaskReady(taskId));

   std::promise<void> followUpCompleted;
   manager.addTask(
      "follow-up",
      "info",
      []() -> tbs::TaskResult {
         return tbs::TaskResult::create(tbs::OperationStatus::SUCCESS, "ok");
      },
      [&](const tbs::TaskMetadata&) { followUpCompleted.set_value(); });
   EXPECT_TRUE(followUpCompleted.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready);
}

}  // namespace
