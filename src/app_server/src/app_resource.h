#pragma once

#include <memory>
#include <string>
#include <map>
#include <tobasa/span.h>

namespace tbs {
namespace app {

class Resource
{
public:

   static nonstd::span<const unsigned char> get(const std::string& path, const std::string& searchContext);
   static std::string getString(const std::string& path, const std::string& searchContext);

   static bool exists(const std::string& path, const std::string& searchContext);
   
   static void add(const std::string& path, const std::string& content);

   struct Object
   {
      Object() {}
      Object(const std::string& path, const std::string& content);
      ~Object();

      std::string name;
      std::string path;
      std::string content;
   };
   using ObjectPtr = std::shared_ptr<Object>;
};


} // namespace app
} // namespace tbs