#pragma once 

#include <tobasa/json.h>

namespace tbs {

enum class OperationStatus
{
   SUCCESS = 0,
   PENDING = 1,
   FAILURE = 2
};

/**
 * @class TaskResult
 * @brief Simple structure to hold result of any operation.
 */
struct TaskResult
{
   Json container;
   std::string message;
   int code = -1;
   std::string status = "None";

   bool empty()
   {
      return ( container.empty() && message.empty() );
   }

   bool isError()
   {
      return code == static_cast<int>(OperationStatus::FAILURE);
   }

   bool isPending()
   {
      return code == static_cast<int>(OperationStatus::PENDING);
   }

   bool isSuccess()
   {
      return code == static_cast<int>(OperationStatus::SUCCESS);
   }   

   static TaskResult create(OperationStatus status, const std::string& message, const Json& val = {})
   {
      std::string statusText = "None";

      if (status == OperationStatus::SUCCESS)
         statusText = "Success";

      if (status == OperationStatus::PENDING)
         statusText = "Pending";

      if (status == OperationStatus::FAILURE)
         statusText = "Failure";

      TaskResult result;
      result.container = val;
      result.message   = message;
      result.code      = (int) status;
      result.status    = statusText;

      return result;
   }
}; 
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TaskResult, container, message, code, status)

} // namespace tbs
