#pragma once

#include <string>
#include <vector>
#include <memory>

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/** 
 * \brief Field, name-value storage
 */
class Field
{
private:
   std::string _name  {};
   std::string _value {};

public:
   Field() = default;
   virtual ~Field() = default;

   Field(const std::string& name, const std::string& value);
   virtual void name(const std::string& name);
   void value(const std::string& value);
   std::string name() const;
   std::string value() const;

   const std::string& nameRef() const;
   const std::string& valueRef() const;

   bool valid() const;
};
using FieldPtr = std::shared_ptr<Field>;


/** 
 * \brief Collection of Fields
 */
class FieldCollection
{
protected:
   std::vector<FieldPtr> _fields;

public:
   FieldCollection() = default;
   virtual ~FieldCollection() = default;

   //! Add field to collection
   // this way we allow more than one header with the same name
   void add(const std::string& fieldName, const std::string& fieldValue);

   //! Set the only one field with name 'name' to 'value'
   // this way we only allow one header with the that name
   void set(const std::string& fieldName, const std::string& fieldValue);

   //! Find a field by name, return nullptr if not found
   virtual FieldPtr find(const std::string& fieldName);

   bool hasField(const std::string& fieldName);

   //! Get value of a field, return empty string if not found
   std::string value(const std::string& fieldName);

   //! Check if fields is empty
   bool empty();

   //! Get total fields
   size_t size();

   std::vector<FieldPtr> getValues();
};




/** 
 * \brief Collection of FieldLowerCase
 */
class FieldLowerCaseCollection
{
protected:
   std::vector<FieldPtr> _fields;

public:
   FieldLowerCaseCollection() = default;
   virtual ~FieldLowerCaseCollection() = default;

   //! Add field to collection
   // this way we allow more than one header with the same name
   void add(const std::string& fieldName, const std::string& fieldValue);

   //! Set the only one field with name 'name' to 'value'
   // this way we only allow one header with the that name
   void set(const std::string& fieldName, const std::string& fieldValue);

   //! Find a field by name, return nullptr if not found
   virtual FieldPtr find(const std::string& fieldName);

   bool hasField(const std::string& fieldName);

   //! Get value of a field, return empty string if not found
   std::string value(const std::string& fieldName);

   //! Check if fields is empty
   bool empty();

   //! Get total fields
   size_t size();

   const std::vector<FieldPtr>& getValues() const;
};

/** @}*/

} // namespace http
} // namespace tbs