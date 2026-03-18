#include "tobasasql/sql_parameter_rewriter.h"

namespace tbs {
namespace sql {


std::string SqlParameterRewriter::rewrite(const std::string& sql) 
{
   _params.clear();

   std::string result;
   result.reserve(sql.size());

   State state = Normal;
   std::string dollarTag;

   for (size_t i = 0; i < sql.size(); ++i) 
   {
      char c = sql[i];

      if (state == Normal) 
      {
         // --- comments ---
         if (c == '-' && i + 1 < sql.size() && sql[i+1] == '-') {
            state = LineComment; result += c; continue;
         }
         if (c == '/' && i + 1 < sql.size() && sql[i+1] == '*') {
            state = BlockComment; result += c; continue;
         }

         // --- strings/identifiers ---
         if (c == '\'') { state = SingleQuote; result += c; continue; }
         if (c == '"')  { state = DoubleQuote; result += c; continue; }

         // --- dollar-quote start ($tag$...$tag$) ---
         if (c == '$') 
         {
            size_t j = i + 1;
            while (j < sql.size() && (isalnum((unsigned char)sql[j]) || sql[j] == '_')) j++;
            if (j < sql.size() && sql[j] == '$') 
            {
               dollarTag = sql.substr(i, j - i + 1);
               state = DollarQuote;
               result += dollarTag;
               i = j;
               continue;
            }
         }

         // --- param detection ---
         if (c == ':' && !(i + 1 < sql.size() && sql[i+1] == ':')) 
         {
            size_t j = i + 1;
            if (j < sql.size() && (isalpha((unsigned char)sql[j]) || sql[j] == '_')) 
            {
               size_t start = j;
               while (j < sql.size() && (isalnum((unsigned char)sql[j]) || sql[j] == '_')) j++;
               std::string name = sql.substr(start, j - start);
               _params.push_back(name);
               result += makePlaceholder((int)_params.size());
               i = j - 1;
               continue;
            }
         }

         // normal char
         result += c;
      }
      else if (state == SingleQuote) 
      {
         result += c;
         if (c == '\'' && !(i+1 < sql.size() && sql[i+1] == '\'')) {
            state = Normal;
         } 
         else if (c == '\'' && i+1 < sql.size() && sql[i+1] == '\'') {
            result += sql[i+1]; i++;
         }
      }
      else if (state == DoubleQuote) 
      {
         result += c;
         if (c == '"') state = Normal;
      }
      else if (state == LineComment) 
      {
         result += c;
         if (c == '\n') state = Normal;
      }
      else if (state == BlockComment) 
      {
         result += c;
         if (c == '*' && i+1 < sql.size() && sql[i+1] == '/') {
            result += '/'; i++; state = Normal;
         }
      }
      else if (state == DollarQuote) 
      {
         result += c;
         if (c == '$' && sql.compare(i - dollarTag.size() + 1, dollarTag.size(), dollarTag) == 0) {
            state = Normal;
         }
      }
   }

   return result;
}


const std::vector<std::string>& SqlParameterRewriter::parameters() const 
{
   return _params;
}


std::string SqlParameterRewriter::makePlaceholder(int index) const 
{
   switch (_dbms) 
   {
   case BackendType::pgsql:  return "$" + std::to_string(index);
   case BackendType::mysql:  return "?";
   case BackendType::sqlite: return "?";   // could be "?NNN" if desired
   case BackendType::adodb:
   case BackendType::odbc:
      return "?";//"@p" + std::to_string(index);
   }
   return "?";
}


} // namespace sql
} // namespace tbs