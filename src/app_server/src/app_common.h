#pragma once

#include <string>

namespace tbs {
namespace app {

const std::string APP_NAME = "tobasa_webapp";

std::string dataDir();

std::string configDir();

std::string imageDir();

std::string reportDir();

std::string uploadDir();

std::string sessionDir();

std::string docRoot();

std::string templateDir();

std::string currentWorkingDir();

}} // namespace tbs::app