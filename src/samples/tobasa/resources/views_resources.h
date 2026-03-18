#pragma once

#include <string>
#include <tobasa/nonstd_span.hpp>

namespace tbs {
namespace res {

nonstd::span<const unsigned char> getViewsResources(const std::string& name);

} // namespace res
} // namespace tb