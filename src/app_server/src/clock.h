#pragma once

#include <functional>
#include <thread>
#include <asio.hpp>

namespace tbs {

class Clock 
{
public:

   Clock(int intervalSeconds);
   ~Clock();

   void start(std::function<void()> callback);
   void stop();

private:
   void handleTimer(const std::error_code& error);


   asio::io_context _ioContext;
   asio::steady_timer _timer;
   asio::thread_pool _workerPool;
   int _interval;
   bool _stopped;
   std::thread _ioContextThread;
   std::function<void()> _callback;

   bool _ioContextStarted;
};

}  // namespace tbs
