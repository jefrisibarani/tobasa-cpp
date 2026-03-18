#pragma once 

#include <optional>
#include <chrono>
#include "tobasa/json.h"
#include "tobasa/task_result.h"

namespace tbs {

/**
 * @class TaskMetadata
 * @brief Hold basic information about one task.
 *
 * This struct store task info like name, status, start time, end time,
 * and the result of the operation. It is used by TaskManager to track
 * progress and store result for each background task.
 */
struct TaskMetadata 
{
   int id;              ///< Unique ID for the task
   std::string name;    ///< Task name (for log or display)
   std::string status;  ///< Current status text (Pending, Running, Completed, None)
   std::string info;    ///< Extra info about this task
   TaskResult result;   ///< Result of the task (success, failure, etc)

   /// Start and end time of the task
   std::optional<std::chrono::system_clock::time_point> startTime;
   std::optional<std::chrono::system_clock::time_point> endTime;
};
using TaskMetadataPtr = std::shared_ptr<TaskMetadata>;

/**
 * @class TaskMetadataDto
 * @brief DTO (Data Transfer Object) for sending task data to client or API.
 *
 * This version is more friendly for API or frontend side,
 * because all data is converted to string (for JSON or text display).
 */
struct TaskMetadataDto
{
   int         id;
   int         taskId;
   std::string name;
   std::string status;
   std::string info;
   std::string startTime;
   std::string endTime;
   std::string duration;
   std::string userId;
   std::string appModule;
   std::string resultStatus;
   std::string resultMessage;
   int         resultCode;
   
   /// @brief Convert TaskMetadata to TaskMetadataDto
   static TaskMetadataDto fromTaskMetadata(TaskMetadataPtr data);
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TaskMetadataDto, id, taskId, name, status, info, startTime, endTime,
   duration, userId, appModule, resultStatus, resultMessage, resultCode)

} // namespace tbs
