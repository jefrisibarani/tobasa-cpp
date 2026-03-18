#include "tobasahttp/status_codes.h"

namespace tbs {
namespace http {

HttpStatus::HttpStatus() {}

HttpStatus::HttpStatus(StatusCode code, const std::string& myreason)
   : _code(code)
   , _myReason(myreason) {}

void HttpStatus::code(StatusCode code)
{
   _code = code;
}

std::string HttpStatus::codeStr()
{
   auto code = static_cast<uint16_t>(_code);
   return  std::to_string(code);
}

void HttpStatus::reason(const std::string& myreason)
{
   _myReason = myreason;
}

StatusCode HttpStatus::code() const
{
   return _code;
}

std::string HttpStatus::reason() const
{
   if (!_myReason.empty()) {
      return _myReason;
   }

   switch(_code)
   {
      case StatusCode::CONTINUE:                         // 100
         return "Continue";
      case StatusCode::SWITCHING_PROTOCOLS:              // 101
         return "Switching Protocols";
      case StatusCode::PROCESSING:                       // 102
         return "Processing";
      case StatusCode::EARLY_HINTS:                      // 103
         return "Early Hints";
      case StatusCode::OK:                               // 200
         return "OK";
      case StatusCode::CREATED:                          // 201
         return "Created";
      case StatusCode::ACCEPTED:                         // 202
         return "Created";
      case StatusCode::NONAUTHORITATIVE_INFORMATION:     // 203
         return "Non Authoritative Information";
      case StatusCode::NO_CONTENT:                       // 204
         return "No Content";
      case StatusCode::RESET_CONTENT:                    // 205
         return "Reset Content";
      case StatusCode::PARTIAL_CONTENT:                  // 206
         return "Partial Content";
      case StatusCode::MULTI_STATUS:                     // 207
         return "Multi Status";
      case StatusCode::ALREADY_REPORTED:                 // 208
         return "Already Reported";
      case StatusCode::IM_USED:                          // 226
         return "IM Used";
      case StatusCode::MULTIPLE_CHOICES:                 // 300
         return "Multiple Choices";
      case StatusCode::MOVED_PERMANENTLY:                // 301
         return "Moved Permanently";
      case StatusCode::FOUND:                            // 302
         return "Found";
      case StatusCode::SEE_OTHER:                        // 303
         return "See Other";
      case StatusCode::NOT_MODIFIED:                     // 304
         return "Not Modified";
      case StatusCode::USE_PROXY:                        // 305
         return "Use Proxy";
      case StatusCode::SWITCH_PROXY:                     // 306 RFC 2616, removed
         return "Switch Proxy ";
      case StatusCode::TEMPORARY_REDIRECT:               // 307
         return "Temporary Redirect";
      case StatusCode::PERMANENT_REDIRECT:               // 308
         return "Permanent Redirect";
      case StatusCode::BAD_REQUEST:                      // 400
         return "Bad Request";
      case StatusCode::UNAUTHORIZED:                     // 401
         return "Unauthorized";
      case StatusCode::PAYMENT_REQUIRED:                 // 402
         return "Payment Required";
      case StatusCode::FORBIDDEN:                        // 403
         return "Forbidden";
      case StatusCode::NOT_FOUND:                        // 404
         return "Not Found";
      case StatusCode::METHOD_NOT_ALLOWED:               // 405
         return "Method Not Allowed";
      case StatusCode::NOT_ACCEPTABLE:                   // 406
         return "Not Acceptable";
      case StatusCode::PROXY_AUTHENTICATION_REQUIRED:    // 407
         return "Proxy Authentication Required";
      case StatusCode::REQUEST_TIMEOUT:                  // 408
         return "Request Timeout";
      case StatusCode::CONFLICT:                         // 409
         return "Conflict";
      case StatusCode::GONE:                             // 410
         return "Gone";
      case StatusCode::LENGTH_REQUIRED:                  // 411
         return "Length Required";
      case StatusCode::PRECONDITION_FAILED:              // 412
         return "Precondition Failed";
      case StatusCode::PAYLOAD_TOO_LARGE:                // 413 // RFC 2616, renamed in RFC 7231
         return "Request Entity TooLarge";
      case StatusCode::URI_TOO_LONG:                     // 414 // RFC 7231
         return "Request Uri TooLong";
      case StatusCode::UNSUPPORTED_MEDIA_TYPE:           // 415
         return "Unsupported Media Type";
      case StatusCode::RANGE_NOT_SATISFIABLE:            // 416 // RFC 2616, renamed in RFC 7233
         return "Range Not Satisfiable";
      case StatusCode::EXPECTATION_FAILED:               // 417
         return "Expectation Failed";
      case StatusCode::IM_A_TEAPOT:                      // 418
         return "Im A Teapot";
      case StatusCode::MISDIRECTED_REQUEST:              // 421
         return "Misdirected Request";
      case StatusCode::UNPROCESSABLE_ENTITY:             // 422
         return "Unprocessable Entity";
      case StatusCode::LOCKED:                           // 423
         return "Locked";
      case StatusCode::FAILED_DEPENDENCY:                // 424
         return "Failed Dependency";
      case StatusCode::TOO_EARLY:                        // 425
         return "Too Early";
      case StatusCode::UPGRADE_REQUIRED:                 // 426
         return "Upgrade Required";
      case StatusCode::PRECONDITION_REQUIRED:            // 428
         return "Precondition Required";
      case StatusCode::TOO_MANY_REQUESTS:                // 429
         return "Too ManyRequests";
      case StatusCode::REQUEST_HEADER_FIELDS_TOO_LARGE:  // 431
         return "Request Header Fields Too Large";
      case StatusCode::UNAVAILABLE_FOR_LEGAL_REASONS:    // 451
         return "Unavailable For Legal Reasons";
      case StatusCode::CLIENT_CLOSED_REQUEST:            // 499
         return "Client Closed Request";
      case StatusCode::INTERNAL_SERVER_ERROR:            // 500
         return "Iternal Server Error";
      case StatusCode::NOT_IMPLEMENTED:                  // 501
         return "Not Implemented";
      case StatusCode::BAD_GATEWAY:                      // 502
         return "Bad Gateway";
      case StatusCode::SERVICE_UNAVAILABLE:              // 503
         return "Service Unavailable";
      case StatusCode::GATEWAY_TIMEOUT:                  // 504
         return "Gateway Timeou";
      case StatusCode::HTTP_VERSION_NOT_SUPPORTED:       // 505
         return "Http Version Not Supported";
      case StatusCode::VARIANT_ALSO_NEGOTIATES:          // 506
         return "Variant Also Negotiates";
      case StatusCode::INSUFFICIENT_STORAGE:             // 507
         return "Insufficient Storage";
      case StatusCode::LOOP_DETECTED:                    // 508
         return "Loop Detected";
      case StatusCode::NOT_EXTENDED:                     // 510
         return "Not Extended";
      case StatusCode::NETWORK_AUTHENTICATION_REQUIRED:  // 511
         return "Network Authentication Required";
      case StatusCode::NETWORK_CONNECT_TIMEOUT_ERROR:    // 599
         return "Netwotk Connect Timeout Error";
   }

   return "Unknown";
}

std::string HttpStatus::reasonWithCode() const
{
   auto code = static_cast<uint16_t>(_code);
   return  std::to_string(code) + " "  + reason();
}


} // namespace http
} // namespace tbs