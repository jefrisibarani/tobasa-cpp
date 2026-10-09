#include "tobasa_test_support/environment.h"

#include <cstdlib>
#include <fstream>

namespace tbs {
namespace test_support {
namespace {

std::string trim(const std::string& value)
{
   const auto first = value.find_first_not_of(" \t\r\n");
   if (first == std::string::npos)
      return {};

   const auto last = value.find_last_not_of(" \t\r\n");
   return value.substr(first, last - first + 1);
}

} // namespace

std::string loadDotEnvValue(const std::string& filePath, const std::string& name)
{
   std::ifstream envFile(filePath);
   if (!envFile)
      return {};

   std::string line;
   while (std::getline(envFile, line))
   {
      line = trim(line);
      if (line.empty() || line[0] == '#')
         continue;

      const auto separator = line.find('=');
      if (separator == std::string::npos || trim(line.substr(0, separator)) != name)
         continue;

      auto value = trim(line.substr(separator + 1));
      if (value.size() >= 2 &&
          ((value.front() == '"' && value.back() == '"') ||
           (value.front() == '\'' && value.back() == '\'')))
      {
         value = value.substr(1, value.size() - 2);
      }
      return value;
   }

   return {};
}

std::string loadEnvironmentValue(
   const std::string& name,
   const std::string& dotEnvFilePath)
{
   const char* environmentValue = std::getenv(name.c_str());
   if (environmentValue != nullptr && environmentValue[0] != '\0')
      return environmentValue;

   return loadDotEnvValue(dotEnvFilePath, name);
}

ScopedEnvironmentVariable::ScopedEnvironmentVariable(
   const std::string& name,
   const std::string& value)
   : _name {name}
   , _hadPreviousValue {std::getenv(name.c_str()) != nullptr}
{
   if (_hadPreviousValue)
      _previousValue = std::getenv(name.c_str());

#if defined(_WIN32)
   if (_putenv_s(_name.c_str(), value.c_str()) != 0)
      throw std::runtime_error("Failed to set process environment variable");
#else
   if (::setenv(_name.c_str(), value.c_str(), 1) != 0)
      throw std::runtime_error("Failed to set process environment variable");
#endif
}

ScopedEnvironmentVariable::~ScopedEnvironmentVariable() noexcept
{
#if defined(_WIN32)
   _putenv_s(_name.c_str(), _hadPreviousValue ? _previousValue.c_str() : "");
#else
   if (_hadPreviousValue)
      ::setenv(_name.c_str(), _previousValue.c_str(), 1);
   else
      ::unsetenv(_name.c_str());
#endif
}

} // namespace test_support
} // namespace tbs