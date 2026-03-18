#include <filesystem>
#include <tobasa/util_string.h>
#include "tobasahttp/multipart.h"

namespace tbs {
namespace http {

namespace fs = std::filesystem;

/*

POST /upload HTTP/1.1
Host: example.com
Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW
Content-Length: 356

------WebKitFormBoundary7MA4YWxkTrZu0gW
Content-Disposition: form-data; name="text_field"

This is text content.
------WebKitFormBoundary7MA4YWxkTrZu0gW
Content-Disposition: form-data; name="file_field"; filename="example.txt"
Content-Type: text/plain

This is the content of the file.
------WebKitFormBoundary7MA4YWxkTrZu0gW--


*/


void MediaType::parse(const std::string& text)
{
   auto token = util::split(text, ';');
   for (size_t i=0; i < token.size(); ++i)
   {
      if (i==0)
      {
         // Parse type and subtype
         auto& tok = token.at(i);
         auto pos = tok.find_first_of('/');
         if( pos != std::string::npos )
         {
            subType = tok.substr(pos+1);
            type    = tok.substr(0, (tok.size()-1) - subType.size() );
         }
      }
      else
      {
         // Parse parameters

         // parameter      = token "=" ( token / quoted-string )
         // Note: Unlike some similar constructs in other header fields,
         // media type parameters do not allow whitespace
         // (even "bad" whitespace) around the "=" character.
         auto& tok = token.at(i);
         auto pos  = tok.find_first_of('=');
         if ( pos != std::string::npos )
         {
            auto val  = tok.substr(pos+1);
            util::replaceAll(val, std::string{"\""}, std::string());

            auto name = tok.substr(0, (tok.size()-1) - val.size() );
            
            val  = util::trim(val);
            name = util::trim(name);

            _fields.emplace_back(std::make_shared<Field>(name,val));
         }
      }
   }
}

bool MediaType::valid()
{
   return (!type.empty());
}

std::string MediaType::fullType()
{
   return type + "/" + subType;
}

std::string Disposition::name()
{
   return _name;
}

bool Disposition::valid()
{
   return !_name.empty();
}

bool Disposition::parse(const std::string& text)
{
   // sample:
   // Content-Disposition: form-data; name="profileImage"; filename="/D:/tmp/_miscs/pics/pedrosa.JPG" 
   // Note: we parse       form-data; name="profileImage"; filename="/D:/tmp/_miscs/pics/pedrosa.JPG" 
   auto token = util::split(text, ';');
   for (size_t i=0; i < token.size(); ++i)
   {
      if (i==0) {
         _name = token.at(i);
      }
      else
      {
         // content-disposition = "Content-Disposition" ":"
         //                         disposition-type *( ";" disposition-parm )
         auto& tok = token.at(i);
         auto pos  = tok.find_first_of('=');
         if( pos != std::string::npos )
         {
            auto val  = tok.substr(pos+1);
            auto name = tok.substr(0, (tok.size()-1) - val.size() );

            // Note: Is it ok to remove all quote?
            util::replaceAll(val, std::string{"\""}, std::string());
            
            // trim whitespaces
            val  = util::trim(val);
            name = util::trim(name);

            _fields.emplace_back(std::make_shared<Field>(name,val));
         }
      }
   }
   
   return true;
}

bool MultipartBody::empty()
{
   return _parts.size() == 0;
}

void MultipartBody::add(MultipartBody::PartPtr part)
{
   _parts.emplace_back(std::move(part));
}

MultipartBody::PartPtr MultipartBody::find(const std::string& name)
{
   for (auto p: _parts)
   {
      if (p->name == name)
         return p;
   }
   return nullptr;
}

std::string MultipartBody::value(const std::string& partName)
{
   for (auto p: _parts)
   {
      if (p->name == partName)
         return p->body;
   }
   return {};
}

void MultipartBody::cleanup(bool removeFiles)
{
   for (auto& p : _parts)
   {
      if (p->isFile)
      {
         if (p->ofs.is_open()) {
            p->ofs.close();
         }
         if (removeFiles && !p->location.empty()) {
            try {
               fs::remove(p->location);
            } catch (...) {
               // ignore delete errors
            }
         }
      }
   }
   _parts.clear();
}


} // namespace http
} // namespace tbs