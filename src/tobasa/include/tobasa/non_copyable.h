#pragma once

namespace tbs {

/**
 * @ingroup TBS
 * Base class for types that cannot be copied.
 *
 * Inheriting from this class deletes the copy constructor and copy
 * assignment operator of the derived type.
 */
class NonCopyable
{
protected:
   NonCopyable() = default;
   ~NonCopyable() = default;
   NonCopyable(const NonCopyable&) = delete;
   const NonCopyable& operator=(const NonCopyable&) = delete;
};

} // namespace tbs