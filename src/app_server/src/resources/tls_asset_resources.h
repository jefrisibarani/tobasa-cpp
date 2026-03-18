#pragma once

#include <string>
#include <tobasa/span.h>

namespace tbs {
namespace res {

nonstd::span<const unsigned char> getTlsAssetResources(const std::string& name);

} // namespace res
} // namespace tbs