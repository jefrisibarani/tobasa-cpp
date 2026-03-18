#pragma once

#include <tobasahttp/server/common.h>
#include <tobasahttp/multipart_parser.h>
#include "tobasaweb/middleware.h"

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */

/**
 * \brief Options for the MultipartMiddleware.
 */
struct MultipartMiddlewareOption
{
   std::string temporaryDir;
};

/**
 * \brief Functor for configuring MultipartMiddlewareOption members.
 */
using MultipartMiddlewareOptionBuilder =
   std::function<void(MultipartMiddlewareOption& option)>;


/**
 * @class MultipartMiddleware
 * @brief Middleware for parsing multipart/form-data uploads.
 *
 * MultipartMiddleware detects requests with Content-Type multipart/form-data
 * and uses MultipartBodyReader + MultipartParser to stream and parse the body.
 *
 * - Configures boundary, content-length, or chunked mode.
 * - Feeds incoming data into the parser via BodyReader callbacks.
 * - Builds a structured MultipartBody attached to the request.
 * - Returns RequestStatus::async while parsing; resumes pipeline on completion.
 *
 * Controllers and later middleware can then access parsed form fields and
 * uploaded files directly from the request object, without handling raw bytes.
 */

class MultipartMiddleware
   : public Middleware
{
public:
   MultipartMiddleware();
   virtual ~MultipartMiddleware() = default;

   /**
    * @brief Entry point for multipart middleware.
    *
    * If the request is multipart/form-data, this method:
    * - Configures and starts a MultipartParser.
    * - Attaches a DataHandler to the BodyReader for streaming parsing.
    * - Returns RequestStatus::async so the server waits until parsing is done.
    *
    * Once parsing completes, it resumes the middleware pipeline by invoking
    * the next handler and calling HttpContext::complete().
    *
    * If the request is not multipart, it simply calls next(context).
    *
    * @param context Current HTTP context (request + response).
    * @return RequestStatus::async if parsing multipart, otherwise result of next().
    */
   virtual http::RequestStatus invoke(const http::HttpContext& context);
   
   /**
    * \brief Sets middleware options for the MultipartMiddleware.
    * \param option The option to be set
    */   
   void option(MultipartMiddlewareOption option);

protected:
   MultipartMiddlewareOption _option;
   //std::unique_ptr<http::parser::MultipartParser> _parser {nullptr};
   //std::unique_ptr<http::MultipartBody> _multipartBody {nullptr};
};

/** @}*/

} // namespace http
} // namespace tbs