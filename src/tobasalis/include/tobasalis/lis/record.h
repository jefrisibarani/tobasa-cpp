#pragma once

#include <memory>
#include <vector>
#include <string>

namespace tbs {
namespace lis {

/** \ingroup LIS
 * Record - Base class for all Record.
 * Holds one record
 */
class Record;
using RecordPtr = std::shared_ptr<Record>;

class Record
{
public:
   Record();
   Record(const std::string& lisString);
   virtual ~Record() = default;

   virtual bool fromString(const std::string& lisString = "") = 0;
   virtual std::string toString() = 0;
   virtual std::string recordTypeStr() = 0;

   void addChild(const RecordPtr& child)
   {
      if (!_pChildren) {
         _pChildren = child;
      }
      else
      {
         RecordPtr ch = _pChildren;
         while (ch->_pNext) {
            ch = ch->_pNext;
         }

         ch->_pNext = child;
      }
      child->_pNext = nullptr;
   }

   RecordPtr getParent() const { return _pParent; }
   RecordPtr getNext() const { return _pNext; }
   RecordPtr getChildren() const { return _pChildren; }
   RecordPtr getLastChild()
   {
      if (!_pChildren)
         return nullptr;

      RecordPtr ch = _pChildren;
      while (ch && ch->_pNext)
      {
         ch = ch->_pNext;
      }
      return ch;
   }

   void setParent(const RecordPtr& parent) { _pParent = parent; }
   void setNext(const RecordPtr& next) { _pNext = next; }
   void setChildren(const RecordPtr& child) { _pChildren = child; }

protected:
   RecordPtr _pParent, _pChildren, _pNext;

   const char  CR = '\r';
   std::string _lisString;
   int         _totalField;
   char        _fieldSeparator = '|';
};

} // namespace lis
} // namespace tbs 