#include "tobasa/navigator.h"
#include "tobasa/exception.h"
#include "tobasa/format.h"

namespace tbs {

NavigatorBasic::NavigatorBasic()
   : Navigator()
   , _position(0) {}

void NavigatorBasic::moveNext()
{
   if (_position < maxPositionHandler() - 1)
      _position++;
   else
      throw AppException(tbsfmt::format("maximum position reached: {}",_position), "NavigatorBasic");
}

void NavigatorBasic::movePrevious()
{
   if (_position > 0)
      _position--;
   else
      throw AppException("invalid location index", "NavigatorBasic");
}

void NavigatorBasic::moveFirst()
{
   _position = 0;
}

void NavigatorBasic::moveLast()
{
   _position = maxPositionHandler() - 1;
}

void NavigatorBasic::locate(long newPosition)
{
   if (newPosition > maxPositionHandler() || newPosition < 0)
      throw AppException("invalid location index", "NavigatorBasic");

   _position = newPosition;
}

long NavigatorBasic::position() const
{
   return _position;
}

bool NavigatorBasic::isBof() const
{
   return (!maxPositionHandler() || _position < 1);
}

bool NavigatorBasic::isEof() const
{
   return (!maxPositionHandler() || _position == maxPositionHandler());
}


} // namespace tbs