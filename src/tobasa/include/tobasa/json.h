#pragma once

#include <nlohmann/json.hpp>
#include "tobasa/exp_imp.h"

namespace tbs {

using Json = nlohmann::json;

TBS_API std::string cleanJsonException(const Json::exception& ex);

} // namespace tbs