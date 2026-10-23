#pragma once 

#include <optional>
#include <chrono>
#include "tobasa/exp_imp.h"
#include "tobasa/json.h"
#include "tobasa/task_result.h"

namespace tbs {

/** @addtogroup TBS
 * @{
 */

/**
 * @class TaskMetadata
 * Hold basic information about one task.
 * Used by TaskManager to track
 * progress and store result for each background task.
 */
struct TaskMetadata 
{
   int id = 0;                  ///< Unique ID for the task
   std::string name;            ///< Task name (for log or display)
   std::string status = "None"; ///< Current status text (Pending, Running, Completed, None)
   std::string info;            ///< Extra info about this task
   TaskResult result;           ///< Result of the task (success, failure, etc)

   /// Start and end time of the task
   std::optional<std::chrono::system_clock::time_point> startTime;
   std::optional<std::chrono::system_clock::time_point> endTime;
};

/**
 * @class TaskMetadataDto
 * DTO (Data Transfer Object) for sending task data to client or API.
 */
struct TBS_API TaskMetadataDto
{
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
   Json        resultData;
   
   /// Convert TaskMetadata to TaskMetadataDto
   static TaskMetadataDto fromTaskMetadata(const TaskMetadata& data);
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TaskMetadataDto, taskId, name, status, info, startTime, endTime,
   duration, userId, appModule, resultStatus, resultMessage, resultCode, resultData)


/** @}*/

} // namespace tbs
