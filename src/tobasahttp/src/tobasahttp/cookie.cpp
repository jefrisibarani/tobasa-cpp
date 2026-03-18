#include <tobasa/util_string.h>
#include "tobasahttp/util.h"
#include "tobasahttp/cookie.h"

namespace tbs {
namespace http {

namespace
{
   /// Parse name and value from a string in format:  name=value
   void parseNameValue(const std::string& line,
      std::function<void(const std::string,const std::string)> handler )
   {
      auto pos = line.find_first_of('=');
      if ( pos != std::string::npos )
      {
         auto val  = line.substr(pos+1);
         auto name = line.substr(0, (line.size()-1) - val.size() );

         // Note: Is it ok to remove all quote?
         util::replaceAll(val, std::string{"\""}, std::string());
         
         // trim whitespaces
         val  = util::trim(val);
         name = util::trim(name);

         handler(name,val);
      }
   }

} // namespace


void Cookie::parse(const std::string& text)
{
   if (text.length() == 0)
      return;

   // Cookie: name=value; name=value; name=value

   auto tokens = util::split(text, ';');
   for (size_t i=0; i < tokens.size(); ++i)
   {
      auto& line = tokens.at(i);
      parseNameValue(line, 
         [this](const std::string name, const std::string val)
         {
            _fields.emplace_back(std::make_shared<Field>(name,val));
         });
   }
}

void Cookie::fromCollection(Headers& headers)
{
   for (size_t i=0;i<headers.size();i++)
   {
      auto field = headers.field(i);
      if (field !=nullptr && field->nameRef() == "cookie")
      {
         auto& line = field->valueRef();
         parseNameValue(line, 
            [this](const std::string name, const std::string val)
            {
               _fields.emplace_back(std::make_shared<Field>(name,val));
            });
      }
   }
}

} // namespace http
} // namespace tbs