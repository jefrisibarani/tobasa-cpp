#pragma once

#include <array>
#include <chrono>
#include <random>
#include <string>
#include <stdexcept>
#include <iostream>

namespace tbs {

// Note: Taken from
// https://firebase.blog/posts/2015/02/the-2120-ways-to-ensure-unique_68
// https://gist.github.com/mikelehen/3596a30bd69384624c11

class IdGenerator
{
private:
   static constexpr char PUSH_CHARS[65] = "-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
   long long _lastPushTime;
   std::array<int, 12> _lastRandChars;

public:

   IdGenerator() 
      : _lastPushTime(0), _lastRandChars{0} {}

   std::string getId() 
   {
      using namespace std::chrono;

      auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
      bool duplicateTime = (now == _lastPushTime);
      _lastPushTime = now;

      std::array<char, 8> timeStampChars;
      for (int i = 7; i >= 0; --i) {
         timeStampChars[i] = PUSH_CHARS[now % 64];
         now /= 64;
      }

      if (now != 0) 
         throw std::runtime_error("We should have converted the entire timestamp.");

      std::string id(timeStampChars.begin(), timeStampChars.end());

      if (!duplicateTime) 
      {
         std::random_device rd;
         std::mt19937 gen(rd());
         std::uniform_int_distribution<> dis(0, 63);

         for (int i = 0; i < 12; ++i) {
               _lastRandChars[i] = dis(gen);
         }
      } 
      else 
      {
         for (int i = 11; i >= 0 && _lastRandChars[i] == 63; --i) {
               _lastRandChars[i] = 0;
         }
         ++_lastRandChars[11];
      }

      for (int i = 0; i < 12; ++i) {
         id += PUSH_CHARS[_lastRandChars[i]];
      }

      if (id.length() != 20) 
         throw std::runtime_error("Length should be 20.");

      return id;
   }
};

} // namespace tbs