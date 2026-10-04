#pragma once

#include <string>

namespace tbs {
namespace test_support {

/// Returns a named value from a .env file, or an empty string when unavailable.
std::string loadDotEnvValue(const std::string& filePath, const std::string& name);

/// Returns the process environment value, falling back to the .env file.
std::string loadEnvironmentValue(
   const std::string& name,
   const std::string& dotEnvFilePath);

} // namespace test_support
} // namespace tbs