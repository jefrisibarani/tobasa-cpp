#pragma once

#include <iostream>
#include <vector>
#include <string_view>
#include <optional>
#include "tobasahttp/headers.h"
#include "tobasahttp/field.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/**
 * @brief Http Request Path Argument.
 *
 * The PathArgument class extracts named parameters from request paths
 * by matching against a path template.
 *
 * Path templates may include placeholders wrapped in `{}` which bind
 * to corresponding segments in the request path.
 *
 * Examples:
 * ```
 * /users/{id:int}
 * /product/{sku:alnum}
 * /media/{file:string}
 * /posts/{slug:slug}
 * /session/{token:uuid}
 * ```
 *
 * Placeholders can optionally declare a type:
 *   - `{id}` or `{id:string}` : defaults to type "string" (any non-empty segment)
 *   - `{id:int}`              : must consist of digits only
 *   - `{name:alpha}`          : must consist of letters only
 *   - `{username:alnum}`      : letters + digits
 *   - `{hash:hex}`            : must consist of hexadecimal digits
 *   - `{useruuid:uuid}`       : RFC4122 UUID format
 *   - `{slug:slug}`           : lowercase letters, digits, '-' or '_'
 *
 * Usage example:
 * ```
 * auto pathArg = std::make_shared<PathArgument>();
 * pathArg->parse("/version/{id:int}/build/{name:alpha}", "/version/42/build/john");
 *
 * if (pathArg->match()) {
 *    auto id   = pathArg->get("id");   // "42"
 *    auto name = pathArg->get("name"); // "john"
 * }
 * ```
 *
 * @note
 * - Leading/trailing slashes are normalized away before matching.
 * - If validation fails or structure mismatches, `match()` returns false 
 *   and and the server should respond with HTTP 404 (Not Found)
 */
class PathArgument
   : public FieldCollection
{
public:
   PathArgument()
   {}
   ~PathArgument() = default;

   /**
    * @brief Parse a request path using a template and extract parameters.
    *
    * Matches a path template (e.g. `/version/{id}/build/{name}`) against
    * a request path (e.g. `/version/42/build/john`). If the structure matches,
    * parameters are extracted into `_fields`.
    *
    * Placeholders may declare a type (default is "string"):
    *   - `{id}` or `{id:string}` → any non-empty value
    *   - `{id:int}`              → digits only
    *   - `{name:alpha}`          → letters only
    *   - `{hash:hex}`            → hexadecimal digits
    *   - `{slug:slug}`           → lowercase letters, digits, '-' or '_'
    *
    * @param pathTemplate Path template with optional typed placeholders.
    * @param requestPath  Actual HTTP request path.
    */
   void parse(const std::string& pathTemplate, const std::string& requestPath);

   /**
    * Get parameter value.
    * \param name parameter name
    * \return std::optional<std::string>
    */
   std::optional<std::string> get(const std::string& name) const;

   /**
    * Get parameter by position.
    * \param position zero based index
    * \return std::optional<FielPtr>
    */
   std::optional<FieldPtr> get(int32_t position) const;

   /**
    * Returns true, when parsing completed, and pathTemplate matched request path
    */
   bool match();

   std::string templatePath() { return _pathTemplate; }
   std::string requestPath() { return _requestPath; }

private:
   std::string _pathTemplate  {};
   std::string _requestPath   {};
   bool        _match         {false};
};

using PathArgumentPtr = std::shared_ptr<PathArgument>;

/** @}*/

} // namespace http
} // namespace tbs