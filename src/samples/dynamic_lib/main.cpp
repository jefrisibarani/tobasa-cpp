#include <tobasa/dynamic_lib.h>
#include <iostream>

#if defined(_WIN32)
   const char* LIB_NAME = "user32.dll";   // system library on Windows
#elif defined(__linux__)
   const char* LIB_NAME = "libm.so.6";    // system math library on Linux
#elif defined(__APPLE__)
   const char* LIB_NAME = "libm.dylib";   // macOS math library
#else
   #error Unsupported platform
#endif

int main()
{
   using namespace tbs;

   try 
   {
      std::cout << "Loading library: " << LIB_NAME << "\n";

      DynamicLib lib(LIB_NAME);
      if (!lib.isLoaded()) {
         std::cerr << "Failed to load: " << lib.getLastError() << "\n";
         return 1;
      }

   #if defined(_WIN32)
      // Example 1: Win32 API function (MessageBoxA, WINAPI = __stdcall)
      using MsgBoxType = DYN_FUNC_T(int, HWND, LPCSTR, LPCSTR, UINT);
      DynamicSymbol<MsgBoxType> msgBox(lib, "MessageBoxA");
      
      if (msgBox.isValid()) {
         msgBox((HWND)0, "Hello from DynamicLib", "Demo", MB_OK);
      } else {
         std::cerr << "MessageBoxA not found: " << lib.getLastError() << "\n";
      }
   #else
      // Example 2: Math library function (cosine)
      using CosFunc = double(*)(double);

      // Optional safe way (DynamicSymbol)
      DynamicSymbol<CosFunc> cosSym(lib, "cos");
      if (cosSym.isValid()) {
         std::cout << "cos(0) = " << cosSym(0.0) << "\n";
      } else {
         std::cerr << "cos not found: " << lib.getLastError() << "\n";
      }

      // Strict way (requireSymbol)
      CosFunc cosReq = lib.requireSymbol<CosFunc>("cos");
      std::cout << "cos(3.14159) = " << cosReq(3.14159) << "\n";
   #endif

      lib.unload();
      std::cout << "Library unloaded.\n";
   }
   catch (const std::exception& ex) {
      std::cerr << "Exception: " << ex.what() << "\n";
      return 1;
   }

   return 0;
}