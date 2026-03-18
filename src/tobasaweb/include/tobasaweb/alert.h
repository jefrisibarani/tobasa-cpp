#pragma once

#include <string>

namespace tbs {
namespace web {

class Alert;

using AlertPtr = std::shared_ptr<Alert>;

/**
 * Manages HTTP session alerts.
 * Handles and manages informational messages specific to an HTTP session.
 */
class Alert
{
public:

   static inline std::string LOC_PAGE    = "Page";
   static inline std::string LOC_FORM    = "Form";
   static inline std::string LOC_TOAST   = "Toast";
   static inline std::string TYP_SUCCESS = "alert-success";
   static inline std::string TYP_INFO    = "alert-info";
   static inline std::string TYP_WARNING = "alert-warning";
   static inline std::string TYP_ERROR   = "alert-danger";

   /// Constructs an Alert instance associated with a specific session.
   /// param sessionId The identifier of the session to which the alert belongs.
   Alert(const std::string& sessionId);

   /// Creates an Alert instance associated with a specific session.
   static AlertPtr create(const std::string& sessionId);

   /// Sets a success message for the alert.
   void success(const std::string& message, const std::string& location=Alert::LOC_TOAST, bool autoClose=true);

   /// Sets a info message for the alert.
   void info(const std::string& message, const std::string& location=Alert::LOC_TOAST, bool autoClose=true);

   /// Sets a warning message for the alert.
   void warning(const std::string& message, const std::string& location=Alert::LOC_TOAST, bool autoClose=true);

   /// Sets a error message for the alert.
   void error(const std::string& message, const std::string& location=Alert::LOC_TOAST, bool autoClose=true);

   /// Adds an alert message to the session data.
   void add(const std::string& type, const std::string& message, const std::string& location=Alert::LOC_TOAST, bool autoClose=true);

   /// Checks if there are any alerts in the session data.
   bool isEmpty();

   /// Removes all alert messages from the session data.
   /// This method removing all existing alert messages.
   void removeAll();

private:

   std::string _sessionId;
};

} // namespace web
} // namespace tbs