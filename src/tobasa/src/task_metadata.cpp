#include <tobasa/datetime.h>
#include <tobasa/task_metadata.h>
#include <tobasa/util.h>

namespace tbs {

TaskMetadataDto TaskMetadataDto::fromTaskMetadata(TaskMetadataPtr data)
{
   TaskMetadataDto dto;

   dto.taskId    = data->id;
   dto.name      = data->name;
   dto.status    = data->status;
   dto.info      = data->info;

   // TODO_JEFRI
   dto.userId    = "";
   dto.appModule = "";

   dto.resultMessage  = data->result.message;
   dto.resultStatus   = data->result.status;
   dto.resultCode     = data->result.code;

   if (data->startTime.has_value() && data->endTime.has_value())
   {
      DateTime dtStart(data->startTime.value());
      dto.startTime = dtStart.isoDateTimeString();

      DateTime dtEnd(data->endTime.value());
      dto.endTime = dtEnd.isoDateTimeString();

      using namespace std::chrono;
      auto interval = (dtEnd.timePoint() - dtStart.timePoint()).count();

      dto.duration = util::readMilliseconds(interval);
   }

   return dto;
}

} // namespace tbs
