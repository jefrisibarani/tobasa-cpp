#include <iostream>

#ifdef _WIN32
   #pragma comment(lib, "rpcrt4.lib") 
   #include <windows.h>
   #include <rpcdce.h>
#else
   #include <sstream>
   #include <random>
#endif

#include "tobasa/uuid.h"

namespace tbs {
namespace uuid {

#ifdef _WIN32

std::string generate()
{
   UUID uuid;
   RPC_CSTR  uuid_str;
   std::string result;

   if (UuidCreate(&uuid) != RPC_S_OK)
      std::cout << "couldn't create uuid\nError code" << GetLastError() << std::endl;

   if (UuidToStringA(&uuid, &uuid_str) != RPC_S_OK)
      std::cout << "couldn't convert uuid to string\nError code" << GetLastError() << std::endl;

   result = (char*)uuid_str;
   RpcStringFreeA(&uuid_str);
   return result;
}

#else

static std::random_device              rd;
static std::mt19937                    gen(rd());
static std::uniform_int_distribution<> dis(0, 15);
static std::uniform_int_distribution<> dis2(8, 11);

std::string generate()
{
   std::stringstream ss;
   int i;
   ss << std::hex;
   for (i = 0; i < 8; i++)
   {
      ss << dis(gen);
   }
   ss << "-";
   for (i = 0; i < 4; i++)
   {
      ss << dis(gen);
   }
   ss << "-4";
   for (i = 0; i < 3; i++)
   {
      ss << dis(gen);
   }
   ss << "-";
   ss << dis2(gen);
   for (i = 0; i < 3; i++)
   {
      ss << dis(gen);
   }
   ss << "-";
   for (i = 0; i < 12; i++)
   {
      ss << dis(gen);
   };
   return ss.str();
}   
#endif

} // namespace uuid
} // namespace tbs