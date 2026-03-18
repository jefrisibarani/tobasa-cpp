#pragma once

#include <string_view>

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/** 
 * HTTP methods.
 */
enum class Method : unsigned
{
   UNKNOWNN,
   GET,
   HEAD,
   POST,
   PUT,
   DEL,
   CONNECT,
   OPTIONS,
   TRACE,
   PATCH
};

/** 
 * Convert Method to string.
 * \param method
 * \return std::string_view
 */
inline std::string_view httpMethodToString(Method method)
{
   switch(method)
   {
      case Method::GET:     return "GET";
      case Method::HEAD:    return "HEAD";
      case Method::POST:    return "POST";
      case Method::PUT:     return "PUT";
      case Method::DEL:     return "DELETE";
      case Method::CONNECT: return "CONNECT";
      case Method::OPTIONS: return "OPTIONS";
      case Method::TRACE:   return "TRACE";
      case Method::PATCH:   return "PATCH";
   }

   return "UNKNOWN";
}

/** 
 * Get Method from string view.
 * \param method
 * \return Method
 */
inline Method httpMethodFromString(std::string_view method)
{
   if      (method == "GET")     return Method::GET;
   else if (method == "HEAD")    return Method::HEAD;
   else if (method == "POST")    return Method::POST;
   else if (method == "PUT")     return Method::PUT;
   else if (method == "DELETE")  return Method::DEL;
   else if (method == "CONNECT") return Method::CONNECT;
   else if (method == "OPTIONS") return Method::OPTIONS;
   else if (method == "TRACE")   return Method::TRACE;
   else if (method == "PATCH")   return Method::PATCH;
   else
      return Method::UNKNOWNN;
}

/** @}*/

} // namespace http
} // namespace tbs