#pragma once

#include <memory>
#include <functional>

namespace tbs {

/** @addtogroup TBS
 * @{
 */

/** 
 * @brief Navigator class interface.
 * Provides navigation for row/column based class
 */
class Navigator
{
public:
   /// Move to the next row.
   virtual void moveNext() = 0;

   /// Move to the previous row.
   virtual void movePrevious() = 0;

   /// Move to the first row.
   virtual void moveFirst() = 0;

   /// Move to the last row.
   virtual void moveLast() = 0;

   /// Set current row.
   virtual void locate(long newPosition) = 0;

   /// Get current position
   virtual long position() const = 0;

   /// Is Beginning of File
   virtual bool isBof() const = 0;

   /// Is End of File
   virtual bool isEof() const = 0;
};

using NavigatorPtr = std::shared_ptr<Navigator>;

/** 
 * @brief Basic navigator.
 * Implementation of Navigator
 * Provides navigation for row/column based class
 */
class NavigatorBasic
   : public Navigator
{
public:

   /// Callback type to get maximum position
   using MaxPositionHandler = std::function<long()>;

   /// Constructor
   NavigatorBasic();

   /// Destructor
   virtual ~NavigatorBasic() = default;

   /// Initialize MaxPositionHandler callback
   template< typename MaxPositionHandler >
   void init(MaxPositionHandler && maxPosHandler)
   {
      maxPositionHandler = std::forward<MaxPositionHandler>(maxPosHandler) ;
   }

   /// Move to the next row.
   virtual void moveNext();

   /// Move to the previous row.
   virtual void movePrevious();

   /// Move to the first row.
   virtual void moveFirst();

   /// Move to the last row.
   virtual void moveLast();

   /// Set current row.
   virtual void locate(long newPosition);

   /// Get current position
   virtual long position() const;

   /// Is Beginning of File
   bool isBof() const;

   /// Is End of File
   bool isEof() const;

   /// Callback to get maximum position
   MaxPositionHandler maxPositionHandler;

private:

   /// Position/Row
   long _position;
};

/** @}*/

} // namespace tbs