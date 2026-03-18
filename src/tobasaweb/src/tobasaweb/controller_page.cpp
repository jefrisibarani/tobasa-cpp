#include "tobasaweb/controller_page.h"

namespace tbs {
namespace web {

bool ControllerPage::isLoggedIn()
{
   return false;
}

void ControllerPage::alertSuccess(const std::string& message, const std::string& location, bool autoClose)
{}

void ControllerPage::alertInfo(const std::string& message, const std::string& location, bool autoClose)
{}

void ControllerPage::alertWarning(const std::string& message, const std::string& location, bool autoClose)
{}

void ControllerPage::alertError(const std::string& message, const std::string& location, bool autoClose)
{}

} // namespace web
} // namespace tbs