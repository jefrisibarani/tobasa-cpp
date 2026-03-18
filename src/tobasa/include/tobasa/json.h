#pragma once

#include <nlohmann/json.hpp>

namespace tbs {

using Json = nlohmann::json;

std::string cleanJsonException(const Json::exception& ex);

} // namespace tbs