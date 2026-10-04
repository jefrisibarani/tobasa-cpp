#pragma once

#include <string>

namespace tbs {
namespace test_support {

std::string normalizeTime(const std::string& timeStr);
std::string normalizeDate(const std::string& dateStr);
std::string normalizeDateTime(const std::string& dateTimeStr);

} // namespace test_support
} // namespace tbs