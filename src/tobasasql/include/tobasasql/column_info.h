#pragma once

#include <tobasa/logger.h>
#include "tobasasql/common_types.h"
#include "tobasasql/util.h"

namespace tbs {
namespace sql {

/**
 * \ingroup SQL
 * \brief This class stores SqlTable's column meta informations.
 */
class ColumnInfo
{
public:
   ColumnInfo()
      : _nativeType(0)
      , _typeLength(0)
      , _typeMod(0)
      , _index(0)
      , _typeClass(TypeClass::unknown)
      , _dataType(DataType::unknown)
      , _isPrimaryKey(false)
      , _isAutoIncrement(false)
      , _isNumeric(false)
      , _isReadOnly(false)
      , _needResize(false)
      , _hasDataLookup(false)
      , _hasAlias(false)
      , _useAlias(false) {}

   ~ColumnInfo() = default;

   std::string getName() const            { return _name; }
   std::string getAlias() const           { return _alias; }
   std::string getNativeTypeStr() const   { return _nativeTypeStr; }
   std::string getDisplayTypeName() const { return _displayTypeName; }
   long getNativeType() const             { return _nativeType; }
   TypeClass getTypeClass() const         { return _typeClass; }
   DataType getDataType() const           { return _dataType; }
   long getTypeLength() const             { return _typeLength; }
   long getTypeMod() const                { return _typeMod; }
   int  getIndex() const                  { return _index; }
   bool isPrimaryKey() const              { return _isPrimaryKey; }
   bool isAutoIncrement() const           { return _isAutoIncrement; }
   bool isNumeric() const                 { return _isNumeric; }
   bool isReadOnly() const                { return _isReadOnly; }
   bool isNeedResize() const              { return _needResize; }
   bool hasAlias() const                  { return _hasAlias; }
   bool useAlias() const                  { return _useAlias; }
   bool hasDataLookup() const             { return _hasDataLookup; }
   void setName(const std::string& name)  { _name = name; }
   void setHasDataLookup()                { _hasDataLookup = true; }

   void setAlias(const std::string& alias, bool usealias = true)
   {
      _alias    = alias;
      _hasAlias = true;
      _useAlias = usealias;
   }

   void setNativeTypeStr(const std::string& type)        { _nativeTypeStr = type; }
   void setDisplayTypeName(const std::string& dispName)  { _displayTypeName = dispName; }
   void setNativeType(long type)                         { _nativeType = type; }
   void setTypeClass(TypeClass typeClass)                { _typeClass = typeClass; }
   void setDataType(DataType dataType)                   { _dataType = dataType; }
   void setTypeLength(long val)                          { _typeLength = val; }
   void setTypeMod(long val)                             { _typeMod = val; }
   void setNeedResize(bool val = true)                   { _needResize = val; }
   void setIndex(const int index)                        { _index = index; }
   void setPrimaryKey(bool val = true)                   { _isPrimaryKey = val; }
   void setAutoIncrement(bool val = true)                { _isAutoIncrement = val; }
   void setNumeric(bool val = true)                      { _isNumeric = val; }
   void setNonNumeric(bool val = false)                  { _isNumeric = val; }
   void setReadOnly(bool val = true)                     { _isReadOnly = val; }
   void setPrecision(int val)                            { _precision = val; }
   void setNumericScale(short val)                       { _numericScale = val; }

   int getSize()
   {
      if (_typeLength == -1 && _typeMod > 0)
         return (_typeMod - 4) >> 16;
      else
         return _typeLength;
   }

   int getPrecision() const
   {
      return _precision;
   }

   short getNumericScale() const
   {
      return _numericScale;
   }

private:

   std::string _name;
   std::string _alias;
   std::string _displayTypeName;
   std::string _nativeTypeStr;
   long        _nativeType;
   long        _typeLength;
   long        _typeMod;
   int         _index;
   TypeClass   _typeClass;
   DataType    _dataType;
   short       _numericScale;
   int         _precision;
   bool        _isPrimaryKey;
   bool        _isAutoIncrement;
   bool        _isNumeric;
   bool        _isReadOnly;
   bool        _needResize;
   bool        _hasDataLookup;
   bool        _hasAlias;
   bool        _useAlias;
};

} // namespace sql
} // namespace tbs