#include "tobasa/datetime.h"
#include "tobasa/util.h"
#include "tobasa/logger.h"
#include "tobasa/task_manager.h"

namespace tbs {

// namespace {
// thread_local TaskManager* currentTaskManager = nullptr;
// }

TaskManager::TaskManager(size_t numThreads)
   : _nextTaskId(0), _stop(false), _stopped(false), _shutdownStarted(false)
{
   if (numThreads == 0)
      throw std::invalid_argument("TaskManager requires at least one worker thread");

   auto mainId = util::threadId(std::this_thread::get_id());

   for (size_t i = 0; i < numThreads; ++i)
   {
      _workers.emplace_back([this, mainId]() {
         auto thId = util::threadId(std::this_thread::get_id());
         Logger::logT("[taskmgr] thread id {} created, called from thread id {}", thId, mainId);
         workerThread();
      });
   }
}

TaskManager::~TaskManager()
{
   stop();
}

void TaskManager::requestStop()
{
   // {
   //    std::lock_guard<std::mutex> lock(_taskMutex);

   //    if (!_shutdownStarted)
   //    {
   //       _stop = true;
   //       _shutdownStarted = true;
   //    }
   // }

   // _condition.notify_all();
}

void TaskManager::stop()
{
   //if (currentTaskManager == this)
   if (isWorkerThread())
      throw std::logic_error("TaskManager workers cannot stop their manager");

   if (!_stopped.exchange(true))
   {
      {
         std::lock_guard<std::mutex> lock(_taskMutex);
         _stop = true;
         _shutdownStarted = true;
      }
   }

   _condition.notify_all();

   std::lock_guard<std::mutex> joinLock(_joinMutex);
   for (auto& worker : _workers)
   {
      if (worker.joinable())
      {
         worker.join();
      }
   }
}

void TaskManager::workerThread()
{
   //currentTaskManager = this;
   while (true)
   {
      std::function<void()> task;

      {
         std::unique_lock<std::mutex> lock(_taskMutex);
         _condition.wait(lock, [this] { return _stop || !_tasksQueue.empty(); });

         if (_stop && _tasksQueue.empty())
         {
            //currentTaskManager = nullptr;
            return;
         }

         if (_tasksQueue.empty())
            continue;

         task = std::move(_tasksQueue.front());
         _tasksQueue.pop();
      }

      task();
   }
}

bool TaskManager::isWorkerThread() const
{
   auto currentId = std::this_thread::get_id();

   for (const auto& worker : _workers)
   {
      if (worker.get_id() == currentId)
         return true;
   }

   return false;
}

std::vector<TaskMetadata> TaskManager::getAllTasks()
{
   std::lock_guard<std::mutex> lock(_taskMutex);
   std::vector<TaskMetadata> taskList;
   taskList.reserve(_tasks.size());
   for (const auto& [id, task] : _tasks)
   {
      taskList.push_back(task);
   }
   return taskList;
}

std::optional<TaskMetadata> TaskManager::getTaskById(int taskId)
{
   std::lock_guard<std::mutex> lock(_taskMutex);
   auto it = _tasks.find(taskId);
   if (it == _tasks.end())
      return std::nullopt;

   return it->second;
}

std::string TaskManager::getTaskStatus(int taskId)
{
   std::lock_guard<std::mutex> lock(_taskMutex);
   auto it = _tasks.find(taskId);
   if (it == _tasks.end())
      return "None";

   return it->second.status;
}

std::vector<TaskMetadataDto> TaskManager::getAllTasksData()
{
   std::lock_guard<std::mutex> lock(_taskMutex);

   std::vector<TaskMetadataDto> result;
   result.reserve(_tasks.size());
   for (const auto& [id, task] : _tasks)
   {
      result.emplace_back(TaskMetadataDto::fromTaskMetadata(task));
   }

   return result;
}

bool TaskManager::isTaskReady(int taskId)
{
   std::lock_guard<std::mutex> lock(_taskMutex);
   auto it = _tasks.find(taskId);
   if (it == _tasks.end())
      return false;

   return it->second.status == "Completed";
}

void TaskManager::cleanup()
{
   std::lock_guard<std::mutex> lock(_taskMutex);
   for (auto it = _tasks.begin(); it != _tasks.end(); )
   {
      if (it->second.status == "Completed")
         it = _tasks.erase(it);
      else
         ++it;
   }
}


} // namespace tbs