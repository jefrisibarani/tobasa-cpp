#include "tobasa/util_string.h"
#include "tobasahttp/field.h"

namespace tbs {
namespace http {

Field::Field(const std::string& name, const std::string& value)
   : _name  { name }
   , _value { value } {}

void Field::name(const std::string& name) { _name = name; }
void Field::value(const std::string& value) {  _value = value; }
std::string Field::name() const  { return _name; }
std::string Field::value() const { return _value; }
const std::string& Field::nameRef() const  { return _name; }
const std::string& Field::valueRef() const { return _value; }
bool Field::valid() const { return !_name.empty(); }


// -------------------------------------------------------
void FieldCollection::add(const std::string& fieldName, const std::string& fieldValue)
{
   _fields.emplace_back( std::make_shared<Field>(std::move( fieldName ), std::move(fieldValue) ) );
}

void FieldCollection::set(const std::string& fieldName, const std::string& fieldValue)
{
   if (_fields.empty() || ! hasField(fieldName))
   {
      add(fieldName, fieldValue);
      return;
   }

   // Remove all fields with the given name
   auto it = _fields.begin();
   while (it != _fields.end())
   {
      if (  (*it)->name()  == fieldName )
      {
         it = _fields.erase(it);
      }
      else
      {
         ++it;
      }
   }

   // Add the new field with the given value
   add(fieldName, fieldValue);
}

FieldPtr FieldCollection::find(const std::string& fieldName)
{
   for (auto f: _fields)
   {
      if ( f->name() == fieldName)
         return f;
   }

   return nullptr;
}

bool FieldCollection::hasField(const std::string& fieldName) { return ( find(fieldName) != nullptr ); }

std::string FieldCollection::value(const std::string& fieldName)
{
   for (auto f: _fields)
   {
      if ( f->name() == fieldName)
         return f->value();
   }

   return {};
}

bool FieldCollection::empty() { return _fields.empty(); }

size_t FieldCollection::size() { return _fields.size(); }

std::vector<FieldPtr> FieldCollection::getValues() { return _fields; }


// -------------------------------------------------------
void FieldLowerCaseCollection::add(const std::string& fieldName, const std::string& fieldValue)
{
   _fields.emplace_back( 
      std::make_shared<Field>(std::move(fieldName ), std::move(fieldValue) ) );
}

void FieldLowerCaseCollection::set(const std::string& fieldName, const std::string& fieldValue)
{
   if (_fields.empty() || ! hasField(fieldName))
   {
      add(fieldName, fieldValue);
      return;
   }

   // Remove all fields with the given name
   auto it = _fields.begin();
   while (it != _fields.end())
   {
      if (  util::toLower((*it)->name()) == util::toLower(fieldName) )
      {
         it = _fields.erase(it);
      }
      else
      {
         ++it;
      }
   }

   // Add the new field with the given value
   add(fieldName, fieldValue);
}


FieldPtr FieldLowerCaseCollection::find(const std::string& fieldName)
{
   for (auto f: _fields)
   {
      if ( util::toLower(f->name()) == util::toLower(fieldName))
         return f;
   }

   return nullptr;
}

bool FieldLowerCaseCollection::hasField(const std::string& fieldName) { return ( find(fieldName) != nullptr ); }

std::string FieldLowerCaseCollection::value(const std::string& fieldName)
{
   for (auto f: _fields)
   {
      if ( util::toLower(f->name()) == util::toLower(fieldName))
         return f->value();
   }

   return {};
}

bool FieldLowerCaseCollection::empty() { return _fields.empty(); }

size_t FieldLowerCaseCollection::size() { return _fields.size(); }

const std::vector<FieldPtr>& FieldLowerCaseCollection::getValues() const { return _fields; }

} // namespace http
} // namespace tbs