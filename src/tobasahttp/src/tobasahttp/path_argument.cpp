#include "tobasahttp/util.h"
#include <tobasa/util_string.h>
#include "tobasahttp/path_argument.h"
#include "tobasahttp/validators.h"

namespace tbs {
namespace http {

namespace {

// helper: split into string_view segments (no allocations)
std::vector<std::string_view> splitView(std::string_view s, char delim = '/')
{
   std::vector<std::string_view> result;
   size_t start = 0;
   while (start < s.size())
   {
      size_t end = s.find(delim, start);
      if (end == std::string_view::npos)
         end = s.size();

      auto seg = s.substr(start, end - start);

      if (!seg.empty())   // skip empty parts from "//"
         result.push_back(seg);

      start = end + 1;
   }
   return result;
}

// helper: strip leading/trailing slashes
inline std::string_view trimSlashes(std::string_view s)
{
   while (!s.empty() && s.front() == '/') s.remove_prefix(1);
   while (!s.empty() && s.back()  == '/') s.remove_suffix(1);
   return s;
}

} // anonymous namespace

void PathArgument::parse(const std::string& pathTemplate, const std::string& requestPath)
{
   if ( pathTemplate.empty() || requestPath.empty() )
      return;

   _pathTemplate = pathTemplate;
   _requestPath  = requestPath;

   // normalize
   std::string_view entryPathV   = trimSlashes(pathTemplate);
   std::string_view requestPathV = trimSlashes(requestPath);

   // split into string_view segments
   auto vPath = splitView(entryPathV);
   auto vReal = splitView(requestPathV);

   if (vReal.empty() || vPath.empty())
      return;

   if (vReal.size() != vPath.size())
      return;

   if (vReal[0] != vPath[0])
      return;

   const auto& validators = getValidators();

   // compare each segment and extract argument(s) from path
   for (size_t pos = 0; pos < vPath.size(); ++pos)
   {
      auto segment = vPath[pos];
      auto value   = vReal[pos];

      if (segment.empty() || value.empty())
         return;

      // check for parameter placeholder
      if (segment.front() == '{' && segment.back() == '}')
      {
         std::string_view spec = segment.substr(1, segment.size() - 2);
         if (spec.empty())
            return;

         // split into name[:type]
         size_t colon = spec.find(':');
         std::string_view argName = spec.substr(0, colon);
         std::string_view argType = (colon != std::string_view::npos)
                                      ? spec.substr(colon + 1)
                                      : "string"; // default

         // find validator
         auto it = validators.find(argType);
         if (it == validators.end())
            return; // unknown type -> reject

         if (!it->second(value))
            return; // validation failed

         _fields.emplace_back(std::make_shared<Field>(
            std::string(argName), std::string(value)
         ));
      }
      else
      {
         // non parameter segment must same with real path segment
         if (segment != value )
            return;
      }
   }

   _match = true;
}

bool PathArgument::match()
{
   return _match;
}

std::optional<std::string> PathArgument::get(const std::string& name) const
{
   for (auto f: _fields)
   {
      if ( f->name() == name)
         return f->value();
   }

   return std::nullopt;
}

std::optional<FieldPtr> PathArgument::get(int32_t position) const
{
   if (_fields.size() > 0 &&  position < static_cast<int32_t>(_fields.size()))
      return std::optional<FieldPtr>{_fields.at(position)};
   else
      return std::nullopt;
}

} // namespace http
} // namespace tbs