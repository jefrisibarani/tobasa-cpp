#include "tobasa/datetime.h"
#include "tobasa/util.h"
#include "tobasa/logger.h"
#include "tobasa/task_manager.h"

namespace tbs {

TaskManager::TaskManager(size_t numThreads) 
   : _nextTaskId(0), _stop(false) , _stopped(false)
{

   auto mainId = util::threadId(std::this_thread::get_id());

   for (size_t i = 0; i < numThreads; ++i) 
   {
      _workers.emplace_back(
         [this,mainId]() { 

            auto thId = util::threadId(std::this_thread::get_id());
            Logger::logT("[taskmgr] thread id {} created, called from thread id {}", thId, mainId );

            workerThread(); 
         });
   }
}

TaskManager::~TaskManager() 
{
   if (!_stopped)
      stop();
}

void TaskManager::stop()
{
   {
      std::lock_guard<std::mutex> lock(_taskMutex);
      _stop = true;
   }
   _condition.notify_all();
   for (auto& worker : _workers) 
   {
      if (worker.joinable())
         worker.join();
   }
   _stopped = true;
}


void TaskManager::workerThread() 
{
   while (true) 
   {
      std::function<void()> task;

      {
         std::unique_lock<std::mutex> lock(_taskMutex);
         _condition.wait(lock, [this] { return _stop || !_tasksQueue.empty(); });

         if (_stop && _tasksQueue.empty()) {
            return; // Exit if stopping and no tasks are left
         }

         task = std::move(_tasksQueue.front());
         _tasksQueue.pop();
      }

      task(); // Execute the task
   }
}


std::vector<TaskMetadataPtr> TaskManager::getAllTasks() 
{
   std::lock_guard<std::mutex> lock(_taskMutex);
   std::vector<TaskMetadataPtr> taskList;
   for (const auto& [id, task] : _tasks) 
   {
      taskList.push_back(task);
   }
   return taskList;
}


TaskMetadataPtr TaskManager::getTaskById(int taskId) 
{
   std::lock_guard<std::mutex> lock(_taskMutex);
   if (_tasks.find(taskId) != _tasks.end()) {
      return _tasks[taskId];
   }

   return nullptr;
}

std::string TaskManager::getTaskStatus(int taskId)
{
   std::lock_guard<std::mutex> lock(_taskMutex);
   auto it = _tasks.find(taskId);
   if (it == _tasks.end())
      return "None";
   
   return it->second->status;
}


std::vector<TaskMetadataDto> TaskManager::getAllTasksData()
{
   std::lock_guard<std::mutex> lock(_taskMutex);
   
   std::vector<TaskMetadataDto> result;
   for (const auto& [id, task] : _tasks) 
   {
      TaskMetadataDto item = TaskMetadataDto::fromTaskMetadata(task);
      result.emplace_back(std::move(item));
   }

   return result;
}


bool TaskManager::isTaskReady(int taskId)
{
   std::lock_guard<std::mutex> lock(_taskMutex);
   auto it = _tasks.find(taskId);
   if (it == _tasks.end())
      return false;

   const auto& meta = it->second;
   return (meta->status == "Completed" ||
           meta->result.code == static_cast<int>(OperationStatus::FAILURE) ||
           meta->result.code == static_cast<int>(OperationStatus::SUCCESS));
}


void TaskManager::cleanup() {
    for (auto it = _tasks.begin(); it != _tasks.end(); ) {
        auto& meta = *it->second;
        if (meta.status == "Completed")
            it = _tasks.erase(it);
        else
            ++it;
    }
}


} // namespace tbs