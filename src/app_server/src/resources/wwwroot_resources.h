#pragma once

#include <string>
#include <tobasa/span.h>

namespace tbs {
namespace res {

nonstd::span<const unsigned char> getWwwrootResources(const std::string& name);

} // namespace res
} // namespace tb