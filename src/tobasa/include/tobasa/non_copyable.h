#pragma once

namespace tbs {

class NonCopyable
{
protected:
   NonCopyable() = default;
   ~NonCopyable() = default;
   NonCopyable(const NonCopyable&) = delete;
   const NonCopyable& operator=(const NonCopyable&) = delete;
};

} // namespace tbs