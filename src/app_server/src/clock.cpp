#include <iostream>
#include "clock.h"

namespace tbs {

Clock::Clock(int intervalSeconds)
   : _timer(_ioContext, asio::chrono::seconds(intervalSeconds))
   , _interval(intervalSeconds)
   , _stopped(false) 
   , _ioContextStarted(false)
   , _workerPool(1)
{
}

Clock::~Clock() 
{
   // Ensure the IO context is stopped and the thread is joined
   stop();  
}

void Clock::start(std::function<void()> callback) 
{
   _callback = std::move(callback);
  
   _timer.async_wait(std::bind(&Clock::handleTimer,
                              this,
                              std::placeholders::_1));

   if (!_ioContextStarted)
   {
      _ioContextStarted = true;
      _ioContextThread = std::thread(
         [this] 
         { 
            _ioContext.run(); 
         });
   }
}

void Clock::stop() 
{
   _stopped = true;
  
   if (_ioContextStarted)
   {
      _timer.cancel();
      _ioContext.stop();

      if (_ioContextThread.joinable())
         _ioContextThread.join();
      // Ensure worker pool finishes any posted callbacks
      _workerPool.join();
   }
}

void Clock::handleTimer(const std::error_code& error)
{
   if (_stopped) {
      return;
   }

   if ( !error ) 
   {
      if (_callback) 
      {
         auto cb = _callback; // copy to ensure lifetime
         asio::post(_workerPool, [cb]() { cb(); });
      }
   }

   // Reschedule the _timer to trigger again after the specified interval
   _timer.expires_at(_timer.expiry() + asio::chrono::seconds(_interval));
   
   //start(_callback);  // Start the timer again with the same callback

   _timer.async_wait(std::bind(&Clock::handleTimer,
                              this,
                              std::placeholders::_1));   
}

}  // namespace tbs