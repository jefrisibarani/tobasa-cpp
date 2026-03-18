#include "tobasa/notifier.h"

namespace tbs {

void Notifier::onNotifyTrace(const std::string& message, const std::string& source) const
{
   NotifyEventArgs arg;
   arg.type    = NotificationType::trace;
   arg.message = message;
   arg.source  = source.empty() ? notifierSource : source;

   if (notificationHandler)
      notificationHandler(arg);
}

void Notifier::onNotifyDebug(const std::string& message, const std::string& source) const
{
   NotifyEventArgs arg;
   arg.type    = NotificationType::debug;
   arg.message = message;
   arg.source  = source.empty() ? notifierSource : source;

   if (notificationHandler)
      notificationHandler(arg);
}

void Notifier::onNotifyInfo(const std::string& message, const std::string& source) const
{
   NotifyEventArgs arg;
   arg.type    = NotificationType::info;
   arg.message = message;
   arg.source  = source.empty() ? notifierSource : source;

   if (notificationHandler)
      notificationHandler(arg);
}

void Notifier::onNotifyWarning(const std::string& message, const std::string& source) const
{
   NotifyEventArgs arg;
   arg.type    = NotificationType::warning;
   arg.message = message;
   arg.source  = source.empty() ? notifierSource : source;

   if (notificationHandler)
      notificationHandler(arg);
}

void Notifier::onNotifyError(const std::string& message, const std::string& source) const
{
   NotifyEventArgs arg;
   arg.type    = NotificationType::error;
   arg.message = message;
   arg.source  = source.empty() ? notifierSource : source;

   if (notificationHandler)
      notificationHandler(arg);
}

void Notifier::onNotify(const NotifyEventArgs& arg) const
{
   if (notificationHandler)
      notificationHandler(arg);
}

} // namespace tbs