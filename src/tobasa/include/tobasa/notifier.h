#pragma once

#include <functional>
#include <string>

namespace tbs {

/** @addtogroup TBS
 * @{
 */

/// Notifier notification type.
enum class NotificationType
{
   trace,
   debug,
   info,
   warning,
   error
};

/// Notifier event argument.
struct NotifyEventArgs
{
   NotifyEventArgs()
   {
      type = NotificationType::info;
   }

   NotificationType type;
   std::string      message;
   std::string      source;
   std::string      summary;
};

/// Notification handler.
using NotificationHandler = std::function<void(const NotifyEventArgs&)>;

/// Notifier class.
struct Notifier
{
   /// Process trace notification
   void onNotifyTrace(const std::string& message, const std::string& source = "") const;

   /// Process debug notification
   void onNotifyDebug(const std::string& message, const std::string& source = "") const;

   /// Process message notification
   void onNotifyInfo(const std::string& message, const std::string& source = "") const;

   /// Process warning notification
   void onNotifyWarning(const std::string& message, const std::string& source = "") const;

   /// Process error notification
   void onNotifyError(const std::string& message, const std::string& source = "") const;

   /// Handle notification / call handler
   void onNotify(const NotifyEventArgs& arg) const;

   /// The actual function to execute notification logic.
   NotificationHandler notificationHandler;

   std::string notifierSource;
};

/** @}*/

} // namespace tbs