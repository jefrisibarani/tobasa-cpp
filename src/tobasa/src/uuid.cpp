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
   UUID uuid{};
   RPC_CSTR uuidString = nullptr;

   RPC_STATUS status = UuidCreate(&uuid);
   if (status != RPC_S_OK && status != RPC_S_UUID_LOCAL_ONLY)
      return {};

   status = UuidToStringA(&uuid, &uuidString);
   if (status != RPC_S_OK)
      return {};

   std::string result(reinterpret_cast<const char*>(uuidString));
   RpcStringFreeA(&uuidString);
   return result;
}

#else

static thread_local std::mt19937 gen(std::random_device{}());

std::string generate()
{
   std::stringstream ss;
   std::uniform_int_distribution<> dis(0, 15);
   std::uniform_int_distribution<> dis2(8, 11);

   ss << std::hex;
   for (int i = 0; i < 8; i++)
   {
      ss << dis(gen);
   }

   ss << "-";

   for (int i = 0; i < 4; i++)
   {
      ss << dis(gen);
   }

   ss << "-4";

   for (int i = 0; i < 3; i++)
   {
      ss << dis(gen);
   }

   ss << "-";

   ss << dis2(gen);
   for (int i = 0; i < 3; i++)
   {
      ss << dis(gen);
   }

   ss << "-";

   for (int i = 0; i < 12; i++)
   {
      ss << dis(gen);
   }

   return ss.str();
}
#endif

} // namespace uuid
} // namespace tbs