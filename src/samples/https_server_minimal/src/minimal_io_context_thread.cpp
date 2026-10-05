#include <csignal>
#include <iostream>
#include <utility>

#include <tobasa/datetime.h>
#include <tobasa/logger.h>
#include "tobasahttp/server/http_server.h"

int main()
{
   try
   {
      if (!tbs::DateTime::initTimezoneData())
         return 1;

      using namespace tbs;

      asio::io_context ioContext;

      tbs::Logger::setTarget(new tbs::log::CoutLogSink());
      log::StdoutLogger logger;
      logger.setLevel(log::Level::TraceMask);

      http::SettingsTls settings("0.0.0.0", 8085);
      settings.certificateChainFile("localhost.crt");
      settings.privateKeyFile("localhost.key");
      settings.tmpDhFile("dh2048.pem");

      http::SecureServerDefault serverHttps(ioContext, std::move(settings), logger);
      serverHttps.requestHandler(
         [](const http::HttpContext& context)
         {
            context->response()->content("Hello World!");
            context->response()->httpStatus(http::StatusCode::OK);
            context->response()->setHeaderContentType("text/plain");
            return http::RequestStatus::handled;
         });

      asio::signal_set breakSignals{ ioContext, SIGINT };
      breakSignals.async_wait(
         [&](const asio::error_code& error, int)
         {
            if (!error)
               serverHttps.stop();
         });

      serverHttps.start();
      ioContext.run();
   }
   catch (const std::exception& ex)
   {
      std::cout << ex.what() << "\n";
      return 1;
   }
   catch (...)
   {
      std::cout << "Exception occured\n";
      return 1;
   }

   return 0;
}
