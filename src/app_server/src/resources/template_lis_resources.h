#pragma once

#if defined(TOBASA_USE_LIS_ENGINE)

#include <string>
#include <tobasa/span.h>

namespace tbs {
namespace res {

nonstd::span<const unsigned char> getTemplateLisResources(const std::string& name);

} // namespace res
} // namespace tbs

#endif // TOBASA_USE_LIS_ENGINE