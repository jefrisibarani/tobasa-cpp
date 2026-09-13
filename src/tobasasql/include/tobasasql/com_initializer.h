#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#pragma once

#include <objbase.h>

namespace tbs {

class ComInitializer
{
private:
   bool _initialized = false;

public:
   explicit ComInitializer(bool required)
   {
      if (required)
      {
         HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
         if (FAILED(result))
            throw std::runtime_error("Could not initialize COM for ADO");

         _initialized = true;
      }
   }

   ~ComInitializer()
   {
      if (_initialized)
      {
         CoUninitialize();
      }
   }
};

} // namespace tbs

#endif