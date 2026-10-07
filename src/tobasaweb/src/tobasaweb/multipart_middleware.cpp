#include <tobasa/config.h>
#include <tobasa/path.h>
#include <tobasa/span.h>
#include <tobasahttp/server/common.h>
#include <tobasahttp/request.h>
#include <tobasahttp/multipart_body_reader.h>
#include "tobasaweb/settings_webapp.h"
#include "tobasaweb/middleware.h"
#include "tobasaweb/multipart_middleware.h"

namespace tbs {
namespace web {

MultipartMiddleware::MultipartMiddleware()
{
   _name = "MultipartMiddleware";
}

void MultipartMiddleware::option(MultipartMiddlewareOption option)
{
   _option = std::move(option);
}

http::RequestStatus MultipartMiddleware::invoke(const http::HttpContext& context)
{
   using namespace tbs::http;

   if (!context->request()->hasMultipart())
      return next(context);

   if (!context->getBodyReader())
      throw std::runtime_error("Missing MultipartBodyReader for multipart request");

   auto parser = std::make_shared<http::parser::MultipartParser>(_option.temporaryDir);
   // set id for debugging
   parser->id( tbsfmt::format("C({})_R({})_Async", context->connId(), context->request()->id() ));

   auto contentLength = context->request()->contentLength();
   parser->contentLength(contentLength);

   if ( context->getBodyReader()->chunkedMultipart() )
   {
      // Tell parser to deal with Chunked Multipart
      parser->chunkedMultipart( true );
   }

   auto ctype =  context->request()->contentType();
   if ( !ctype.empty() )
   {
      MediaType media;
      media.parse(ctype);
      if ( media.valid() && util::toLower(media.fullType()) == "multipart/form-data" )
      {
         auto boundary = media.find("boundary");
         if ( boundary && boundary->valid() && parser->applyBoundary(boundary->value()) )
            context->request()->multipartBody(std::make_unique<MultipartBody>());
         else 
            throw std::runtime_error("Invalid multipart boundary");
      }
   }


   // MultipartBodyReader's read() will trigger async call to retrieve all data.
   // we return http::RequestStatus::async, so ServerConnection will not prematurely write() response to client.
   // When parsing done we inform ServerConnection to write() by calling HttpContext's complete()
   //
   // The trick is to call next middleware and informing ServerConnection, after we got all data.
   // We also move parser into this callback
   auto nextHandler = _nextHandler;
   auto dataHandler = 
      [weakContext = std::weak_ptr<http::Context>{context}, 
       nextHandler = std::move(nextHandler), 
       mparser=std::move(parser)](const uint8_t *data, size_t totalData) 
      {
         auto ctx = weakContext.lock();
         if (!ctx)
            return http::parser::Info{false, "HTTP context is no longer available", 0, {}, 0};

         auto info = mparser->parse(data, totalData);
         if (info.success())
         {
            if ( mparser->done() )
            {
               info.message("multipart-done");
               ctx->request()->multipartBody(std::move( mparser->multipartBody() ) );
               ctx->getBodyReader()->done(true);
               if (nextHandler)
               {
                  // resume pipeline
                  auto nextStatus = nextHandler(ctx);
                  ctx->complete(nextStatus);
               } 
               else
                  ctx->complete(); // default ends with RequestStatus::handled
            }
         }
         // when info.error() occurred, ServerConnection will handle the cleanups
         return info;
      };


   context->getBodyReader()->read(std::move(dataHandler));
   context->bodyReaderReady();

   // Instead of next(context), we return async status.
   // This way ServerConnection will not write a response immediately, 
   // But later after we call context->complete() 
   return http::RequestStatus::async;
}

} // namespace http
} // namespace tbs