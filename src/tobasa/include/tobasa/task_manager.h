#pragma once

#include <iostream>
#include <unordered_map>
#include <string>
#include <atomic>
#include <future>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>
#include "tobasa/exp_imp.h"
#include "tobasa/logger.h"
#include "tobasa/task_metadata.h"

namespace tbs {

/** @addtogroup TBS
 * @{
 */

/**
 * @class TaskManager
 * Simple thread pool for running background task.
 *
 * This class can run many task in background using worker thread.
 * Each task has its own metadata and result.
 * You can check task status, result, and timing information later.
 *
 * Example:
 * @code
 * tbs::TaskManager manager(4);
 *
 * int id = manager.addTask("Import DICOM", "",
 *     []() -> TaskResult {
 *         std::this_thread::sleep_for(std::chrono::seconds(2));
 *         return TaskResult::create(OperationStatus::SUCCESS, "Done importing files");
 *     },
 *     [](const TaskMetadata& m) {
 *         std::cout << "Task completed: " << m.name << std::endl;
 *     }
 * );
 *
 * while (!manager.isTaskReady(id)) {
 *     std::cout << "Waiting..." << std::endl;
 *     std::this_thread::sleep_for(std::chrono::milliseconds(300));
 * }
 * @endcode
 */
class TBS_API TaskManager
{
public:

   /// Create task manager with some worker threads.
   /// @param numThreads number of worker threads.
   TaskManager(size_t numThreads);

   /// Destructor, will stop all threads if not stopped yet.
   ~TaskManager();

   /// TODO_JEFRI
   /// Request shutdown without waiting for worker threads to finish.
   /// Pending tasks will be executed before workers exit.
   /// New tasks cannot be added after this call.
   void requestStop();

   /// Request shutdown and wait for all worker threads to finish.
   /// @throws std::logic_error if called from one of this manager's worker threads.
   void stop();

   /**
    * Add new task to be executed in background.
    *
    * @tparam F function type that returns TaskResult.
    * @param taskName name of the task.
    * @param taskInfo extra info about the task.
    * @param task lambda or function to run.
    * @param onCompletion callback after task finish (optional).
    * @return unique id of created task.
    */
   template <typename F>
   int addTask(const std::string& taskName,
      const std::string& taskInfo,
      F&& task,
      std::function<void(TaskMetadata)> onCompletion = nullptr);

   /// Get list of all tasks (running or completed).
   std::vector<TaskMetadata> getAllTasks();

   /// Get task metadata by id.
   /// @param taskId task id.
   /// @return snapshot of TaskMetadata or std::nullopt if not found.
   std::optional<TaskMetadata> getTaskById(int taskId);

   /// Return "Pending", "Running", "Completed" or "None"
   std::string getTaskStatus(int taskId);

   /// Convert all task info to DTO for easier API or JSON use.
   std::vector<TaskMetadataDto> getAllTasksData();

   /// Check if specific task already finished.
   /// @param taskId task id to check.
   /// @return true if finished, false if still running or pending.
   bool isTaskReady(int taskId);

   /// Remove completed tasks
   void cleanup();

private:
   
   /**
    * Worker thread function.
    *
    * This is the main loop for background worker.
    * It waits until there is a new task in queue, then executes it.
    * When stop flag is true and no task left, thread will exit.
    */
   void workerThread();

   bool isWorkerThread() const;

   std::mutex  _taskMutex;
   std::mutex _joinMutex;
   std::condition_variable _condition;
   std::queue<std::function<void()>> _tasksQueue;
   std::vector<std::thread> _workers;
   std::unordered_map<int, TaskMetadata> _tasks;

   std::atomic<int>  _nextTaskId = 0;
   std::atomic<bool> _stop;
   std::atomic<bool> _stopped;
   std::atomic<bool> _shutdownStarted;
};

/// Add a task and assign it a unique ID
template <typename F>
int TaskManager::addTask(
   const std::string& taskName,
   const std::string& taskInfo,
   F&& task,
   std::function<void(TaskMetadata)> onCompletion)
{
   if (_workers.empty()) {
      throw std::invalid_argument("TaskManager requires at least one worker thread");
   }

   int taskId = ++_nextTaskId;

   TaskMetadata meta;
   meta.id     = taskId;
   meta.name   = taskName;
   meta.info   = taskInfo;
   meta.status = "Pending";

   {
      std::lock_guard<std::mutex> lock(_taskMutex);
      if (_stop || _shutdownStarted) {
         throw std::logic_error("TaskManager is stopping; no tasks may be added");
      }
      _tasks[taskId] = meta;
      _tasksQueue.push([this, taskId, task = std::forward<F>(task), onCompletion, meta]() mutable 
      {
         TaskMetadata localMeta = meta;
         localMeta.startTime    = std::chrono::system_clock::now();
         localMeta.status       = "Running";

         {
            std::lock_guard<std::mutex> lock(_taskMutex);
            auto it = _tasks.find(taskId);
            if (it != _tasks.end())
               it->second = localMeta;
         }

         try {
            localMeta.result = task();
         } 
         catch (const std::exception& ex) {
            localMeta.result = TaskResult::create(OperationStatus::FAILURE, ex.what());
         } 
         catch (...) {
            localMeta.result = TaskResult::create(OperationStatus::FAILURE, "Unknown task error");
         }

         localMeta.status = "Completed";
         localMeta.endTime = std::chrono::system_clock::now();

         {
            std::lock_guard<std::mutex> lock(_taskMutex);
            auto it = _tasks.find(taskId);
            if (it != _tasks.end())
               it->second = localMeta;
         }

         if (onCompletion) 
         {
            try {
               onCompletion(localMeta);
            } 
            catch (const std::exception& ex) {
               Logger::logE("[taskmgr] completion callback failed for task {}: {}", taskId, ex.what());
            } 
            catch (...) {
               Logger::logE("[taskmgr] completion callback failed for task {}: unknown error", taskId);
            }
         }
      });
   }

   _condition.notify_one();
   return taskId;
}

/** @}*/

} // namespace tbs

// -------------------------------------------------------
// Example usage
#if 0

#include <iostream>
#include <thread>
#include <iomanip>  // For std::put_time
#include <ctime>    // For std::localtime
#include <tobasa/task_manager.h>

// Function to print time from a time_point
void printTime(std::optional<std::chrono::system_clock::time_point> timePoint) 
{
   if (timePoint.has_value()) 
   {
      std::time_t time = std::chrono::system_clock::to_time_t(timePoint.value());
      std::cout << std::put_time(std::localtime(&time), "%F %T") << std::endl;
   } 
   else {
      std::cout << "N/A" << std::endl;
   }
}

int main() 
{
   using namespace tbs;

   TaskManager taskManager(4);

   auto taskId1 = taskManager.addTask(
      "Send DICOM File", "Some DICOM file info",
      []() -> std::string {
         std::this_thread::sleep_for(std::chrono::seconds(2));  // Simulate task work
         return "DICOM File Sent";
      },

      [](const TaskMetadata& metadata) {
         if (metadata.result.isError())
            std::cerr << "Task [" << metadata.id << "] failed with error: " << metadata.result.message << std::endl;
         else
            std::cout << "Task [" << metadata.id << "] completed with result: " << metadata.result.message << std::endl;

         std::cout << "Start Time: ";
         printTime(metadata.startTime);

         std::cout << "End Time: ";
         printTime(metadata.endTime);
      }
   );

   // Wait for a short period to allow the task to complete
   std::this_thread::sleep_for(std::chrono::seconds(3));  

   // Query the specific task by ID and print the times
   auto task = taskManager.getTaskById(taskId1);
   if (task) 
   {
      std::cout << "Task [" << task->id << "] Start Time: ";
      printTime(task->startTime);

      std::cout << "Task [" << task->id << "] End Time: ";
      printTime(task->endTime);
   }

   return 0;
}

#endif // of 0