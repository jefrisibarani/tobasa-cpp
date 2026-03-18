#include "tobasahttp/util.h"
#include "tobasahttp/query_string.h"

namespace tbs {
namespace http {

void QueryString::parse(const std::string& text)
{
   if (text.length() == 0)
      return;

   using namespace std::literals;
   std::string decodedStr = text;
   std::string_view decoded{decodedStr};

   auto decodedLength = decoded.length();

   while (decodedLength > 0)
   {
      auto delimPos = decoded.find_first_of(";&"sv);
      if (delimPos == std::string_view::npos)
         delimPos = decodedLength;

      auto token    = decoded.substr(0, delimPos);
      auto equalPos = token.find_first_of('=');
      if (equalPos != std::string_view::npos)
      {
         auto field_ = token.substr(0,equalPos);
         auto value_ = token.substr( (equalPos+1), token.size()-field_.size() );

         std::string field {field_};
         std::string value {value_};
         _fields.emplace_back(std::make_shared<Field>( percentDecode(field), percentDecode(value) ));
      }
      else
      {
         std::string field {token};
         std::string value {};
         _fields.emplace_back(std::make_shared<Field>( percentDecode(field), value ));
      }

      if (delimPos == decodedLength)
         decoded.remove_prefix(delimPos);
      else
         decoded.remove_prefix(delimPos+1);

      decodedLength = decoded.length();
   }
}

std::optional<std::string> QueryString::get(const std::string& name) const
{
   for (auto f: _fields)
   {
      if ( f->name() == name)
         return std::optional<std::string>{f->value()};
   }

   return nullptr;
}

std::optional<FieldPtr> QueryString::get(int32_t position) const
{
   if (_fields.size()>0 &&  position < _fields.size())
      return std::optional<FieldPtr>{_fields.at(position)};
   else
      return std::nullopt;
}

} // namespace http
} // namespace tbs