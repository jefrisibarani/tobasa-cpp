#pragma once

#include <sstream>
#include "tobasahttp/type_common.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/// Get string representation.
template <typename T> std::string toString(const T& v)
{
   std::ostringstream ostr;
   ostr << v ;
   return ostr.str() ;
}

/** 
 * Decode a percent-encoded data, return true on success.
 * \param data
 * \param out ouput decoded value
 */
bool percentDecode(const std::string& data, std::string& out);

/** 
 * Decode a percent-encoded data, return decoded value.
 * @param data
 */
std::string percentDecode(const std::string& data);

/** 
 * Extract and get path from uri.
 * @param uri
 * @sa https://www.rfc-editor.org/rfc/rfc3986#section-3.3
 */
std::string urlGetPath(const std::string& uri);

/** 
 * Extract and get query string from uri.
 * @param uri
 * @sa https://www.rfc-editor.org/rfc/rfc3986#section-3.4
 */
std::string urlGetQueryString(const std::string& uri);

/** 
 * Get TimerType as string
 */
std::string timerTypeToString(TimerType timerType);

std::string logHttpTypeInfo(InstanceType _instanceType, bool tlsMode);

bool isCompressible(const std::string& ctype, const std::vector<std::string>& mimetypes);

/** @}*/

} // namespace http
} // namespace tbs