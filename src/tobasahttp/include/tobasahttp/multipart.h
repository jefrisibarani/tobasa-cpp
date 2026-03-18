#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "tobasahttp/headers.h"
#include "tobasahttp/field.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/** 
 * Multipart Media type.
 * @sa https://httpwg.org/specs/rfc7231.html#rfc.section.3.1.1.1
 */
class MediaType
   : public FieldLowerCaseCollection
{
private:
   std::string type     {};
   std::string subType  {};

public:
   MediaType() = default;
   virtual ~MediaType() = default;

   void parse(const std::string& text);
   bool valid();
   std::string fullType();
};

/** 
 * Content disposition.
 * @sa https://www.rfc-editor.org/rfc/rfc6266
 */
class Disposition
   : public FieldLowerCaseCollection
{
private:
   std::string _name;

public:
   Disposition() = default;
   virtual ~Disposition() = default;

   std::string name();
   bool valid();
   bool parse(const std::string& text);
};


/** 
 * Multipart body.
 */
class MultipartBody
{
public:
   MultipartBody() = default;
   ~MultipartBody() = default;

   struct Part
   {
      ~Part(){}

      std::string name        {};
      std::string body        {};
      std::string contentType {};
      std::string fileName    {};
      bool        isFile      {false};
      Headers     headers     {};
      bool        initialized {false};
      std::string location    {};
      std::ofstream ofs; // persistent output stream
   };
   using PartPtr = std::shared_ptr<Part>;

private:
   std::vector<PartPtr> _parts;

public:
   bool empty();
   void add(PartPtr part);
   PartPtr find(const std::string& name);
   std::string value(const std::string& partName);
   const std::vector<PartPtr>& parts() const { return _parts;}
   void cleanup(bool removeFiles);
};

using MultipartBodyPtr  = std::shared_ptr<MultipartBody>;
using MultipartBodyUPtr = std::unique_ptr<MultipartBody>;

/** @}*/

} // namespace http
} // namespace tbs