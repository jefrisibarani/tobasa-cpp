#include <cstring>
#include <string_view>
#include <tobasa/hextodec.h>
#include <tobasa/common.h>
#include <tobasa/util.h>
#include "tobasahttp/util.h"

namespace tbs {
namespace http {

bool percentDecode(const std::string& data, std::string& out)
{
   out.clear();
   out.reserve(data.size());

   for (std::size_t i = 0; i < data.size(); ++i)
   {
      if (data[i] == '%')
      {
         if (i + 2 < data.size())
         {
            int hi = crypt::hexToDec(data[i + 1]);
            int lo = crypt::hexToDec(data[i + 2]);
            if (hi >= 0 && lo >= 0)
            {
               out += static_cast<char>((hi << 4) | lo);
               i += 2;
            }
            else
               return false;
         }
         else
            return false;
      }
      else if (data[i] == '+')
         out += ' ';
      else
         out += data[i];
   }

   return true;
}

std::string percentDecode(const std::string& data)
{
   std::string out;
   if (percentDecode(data, out))
      return out;
   return {};
}

std::string urlGetPath(const std::string& uri)
{
   if (uri.empty())
      return {};

   using namespace std::literals;

   std::string_view targetV {uri};
   auto pos = targetV.find_first_of("?#"sv);

   if (pos == std::string_view::npos)
      return uri;
   else
   {
      auto path = targetV.substr(0,pos);
      return std::string { path };
   }
}

std::string urlGetQueryString(const std::string& uri)
{
   if (uri.empty())
      return {};

   std::string_view targetV {uri};
   auto pos = targetV.find_first_of('?');

   if (pos == std::string_view::npos)
      return {};
   else
   {
      auto qryStr  = targetV.substr(pos+1); // eat all

      // check for any fragment/hash
      auto hashPos = qryStr.find_first_of('#');
      if (hashPos != std::string_view::npos)
      {
         qryStr = qryStr.substr(0,hashPos);
      }

      return std::string { qryStr };
   }
}

std::string timerTypeToString(TimerType timerType)
{
   if (timerType == TimerType::read)
      return "Read";
   else if (timerType == TimerType::write)
      return "Write";
   else if (timerType == TimerType::process)
      return "Process";
   else
      return "Unknown";
}

std::string logHttpTypeInfo(InstanceType _instanceType, bool tlsMode)
{
   if (_instanceType == InstanceType::http_server)
   {
      if (tlsMode)
         return "srv_https";
      else
         return "srv_http";
   }
   else
   {
      if (tlsMode)
         return "cln_https";
      else
         return "cln_http";
   }
}

bool isCompressible(const std::string& ctype, const std::vector<std::string>& mimetypes)
{
   if (util::findPositionInVector(mimetypes, ctype) == tbs::NOT_FOUND)
      return false;
   else
      return true;
}

} // namespace http
} // namespace tbs