#pragma once

#include "tobasaweb/middleware_base.h"

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */

/**
 * Skip invoke() processing callback.
 * The callback should return 'true' to indicate that the current connection will be ignored.
 */
using MiddlewareIgnoreHandler = std::function<bool(const http::HttpContext& context)>;


/**
 * \brief Handles processing of HTTP requests and responses.
 * The Middleware class serves as a processor for HTTP requests and responses
 * within the HttpContext, providing functionality to handle middleware-related tasks.
 */
class Middleware
   : public MiddlewareBase
{
protected:

   /**
    * \brief Represents a request handler in a chain of middleware.
    * If this handler is the last middleware in the chain, the 'next' handler is nullptr,
    * indicating that it should not be invoked.
    */
   http::RequestHandler _nextHandler;

public:
   Middleware() = default;
   virtual ~Middleware() = default;

   /**
    * \brief Performs the actual processing of HTTP request and response within the context.
    * This method must be overridden by derived classes to handle HTTP context processing.
    * After completing the processing of the request or response, the function should call next()
    * to pass control to the next middleware. If not, it should return RequestStatus::handled.
    *
    * \param context The HTTP context containing request and response information.
    * \return The status of the HTTP request processing.
    */
   virtual http::RequestStatus invoke(const http::HttpContext& context) = 0;

   /// Wraps invoke() in a functor object
   http::RequestHandler getHandler()
   {
      using namespace std::placeholders;
      return std::bind(&Middleware::invoke, this, _1);
   }

   /// Set next request handler functor
   void nextHandler(http::RequestHandler handler)
   {
      _nextHandler = std::move(handler);
   }

   /// Call next request handler functor
   http::RequestStatus next(const http::HttpContext& context)
   {
      if (_nextHandler)
         return (_nextHandler)(context);
      else
         return http::RequestStatus::handled;
   }
};


/**
 * Turn RequestHandlerChained into a middleware
 */
class AutoMiddleware final
   : public Middleware
{
private:
   http::RequestHandlerChained _handler;

public:
   AutoMiddleware() = default;
   virtual ~AutoMiddleware() = default;

   AutoMiddleware(http::RequestHandlerChained handler)
   {
      _handler = std::move(handler);
   }

   http::RequestStatus invoke(const http::HttpContext& context)
   {
      auto callNext = [this] (const http::HttpContext& context)
                      {
                         return next(context);
                      };
      return (_handler)(context, std::move(callNext));
   }

};

/** @}*/

} // namespace http
} // namespace tbs