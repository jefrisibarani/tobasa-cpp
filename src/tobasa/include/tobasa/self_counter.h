#pragma once

namespace tbs {

/** 
 * @ingroup TBS
 * Class providing instance Id.
 */
class SelfCounter
{
public:
   SelfCounter()
   {
      ++_id;
      _selfId = _id;
   }

   /// Get Id
   int selfId() { return _selfId; }

private:
   int _selfId = 0;
   inline static int _id = 1;
};

} // namespace tbs