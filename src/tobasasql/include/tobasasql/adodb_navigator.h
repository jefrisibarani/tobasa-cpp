#pragma once

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#include <functional>

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief AdodbResult Navigator class.
 * Provides navigation for AdodbResult
 * \tparam AdodbResulT
 */
template <typename AdodbResulT>
class AdoResultNavigator
{
public:
   /// Constructor.
   AdoResultNavigator()
      : _position(0)
      , _pAdodbResult(nullptr) {}

   virtual ~AdoResultNavigator() = default;

   /// Initialize MaxPositionHandler callback.
   void init(AdodbResulT* pResult)
   {
      _pAdodbResult = pResult;
   }

   /// Move to the next row.
   virtual void moveNext()
   {
      if (_position < _pAdodbResult->_nRows - 1)
      {
         _position++;

         if ( ! _pAdodbResult->_dataCached)
            _pAdodbResult->_pResult->MoveNext();
      }
      else
         throw AppException("invalid location index");
   }

   /// Move to the previous row.
   virtual void movePrevious()
   {
      if (_position > 0)
      {
         _position--;

         if ( ! _pAdodbResult->_dataCached)
            _pAdodbResult->_pResult->MovePrevious();
      }
      else
         throw AppException("invalid location index");
   }

   /// Move to the first row.
   virtual void moveFirst()
   {
      _position = 0;

      if ( ! _pAdodbResult->_dataCached)
         _pAdodbResult->_pResult->MoveFirst();
   }

   /// Move to the last row.
   virtual void moveLast()
   {
      _position = _pAdodbResult->_nRows - 1;

      if ( ! _pAdodbResult->_dataCached)
         _pAdodbResult->_pResult->MoveLast();
   }

   /// Set current row.
   virtual void locate(long newPosition)
   {

      if (newPosition > _pAdodbResult->_nRows || newPosition < 0)
         throw std::exception("invalid location index");

      _position = newPosition;

      if ( ! _pAdodbResult->_dataCached)
      {
         ADODB::PositionEnum newPos = static_cast< ADODB::PositionEnum>(newPosition + 1L);
         _pAdodbResult->_pResult->AbsolutePosition = newPos;
      }
   }

   /// Get current row.
   virtual long position() const
   {

      if (  _pAdodbResult->_dataCached)
         return _position;
      else
      {
         ADODB::PositionEnum curpos = (ADODB::PositionEnum )_pAdodbResult->_pResult->AbsolutePosition;
         ADODB::PositionEnum pos =  static_cast< ADODB::PositionEnum>(curpos - (ADODB::PositionEnum)1L);
         return (long) pos;
      }
   }

   /// Is Beginning of File.
   bool isBof() const
   {
      if (_pAdodbResult->_dataCached)
         return (!_pAdodbResult->_nRows || _pAdodbResult->_currentRow < 1);
      else
         return _pAdodbResult->_pResult->BOF;
   }

   /// Is End of File.
   bool isEof() const
   {
      if (_pAdodbResult->_dataCached)
         return (!_pAdodbResult->_nRows || _position == _pAdodbResult->_nRows);
      else
         return _pAdodbResult->_pResult->EndOfFile;
   }

private:

   /// Position/Row.
   long _position;

   // Pointer to AdodbResult object.
   AdodbResulT *_pAdodbResult;
};

} // namespace sql
} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)