#include "tobasa/dynamic_lib.h"

namespace tbs {

DynamicLib::DynamicLib() : handle(nullptr) {}

DynamicLib::DynamicLib(const std::string& path) : handle(nullptr)
{
   load(path);
}

DynamicLib::~DynamicLib()
{
   unload();
}

DynamicLib::DynamicLib(DynamicLib&& other) noexcept
{
   std::lock_guard<std::mutex> lock(other.mutex);
   handle = other.handle;
   lastError = std::move(other.lastError);
   other.handle = nullptr;
}

DynamicLib& DynamicLib::operator=(DynamicLib&& other) noexcept
{
   if (this != &other) 
   {
      unload();
      std::scoped_lock lock(mutex, other.mutex);
      handle = other.handle;
      lastError = std::move(other.lastError);
      other.handle = nullptr;
   }
   return *this;
}

bool DynamicLib::isLoaded() const
{
   std::lock_guard<std::mutex> lock(mutex);
   return handle != nullptr;
}

bool DynamicLib::load(const std::string& path)
{
   std::lock_guard<std::mutex> lock(mutex);
   unloadLocked();
   clearError();

#if defined(_WIN32) || defined(_WIN64)
   HMODULE h = ::LoadLibraryA(path.c_str());
   if (!h) 
   {
      setLastError("LoadLibrary failed: " + getWin32Error());
      return false;
   }
   handle = static_cast<void*>(h);
#else
   handle = dlopen(path.c_str(), RTLD_LAZY);
   if (!handle) 
   {
      setLastError(std::string("dlopen failed: ") + dlerror());
      return false;
   }
#endif
   return true;
}

void DynamicLib::unload()
{
   std::lock_guard<std::mutex> lock(mutex);
   unloadLocked();
}

void* DynamicLib::getSymbol(const std::string& symbolName)
{
   std::lock_guard<std::mutex> lock(mutex);
   if (!handle) {
      setLastError("getSymbol failed: library not loaded");
      return nullptr;
   }
   clearError();

#if defined(_WIN32) || defined(_WIN64)
   void* sym = reinterpret_cast<void*>(
      ::GetProcAddress(static_cast<HMODULE>(handle), symbolName.c_str()));
   if (!sym) {
      setLastError("GetProcAddress failed: " + getWin32Error());
   }
   return sym;
#else
   dlerror(); // clear old error
   void* sym = dlsym(handle, symbolName.c_str());
   const char* err = dlerror();
   if (err) {
      setLastError(std::string("dlsym failed: ") + err);
      return nullptr;
   }
   return sym;
#endif
}

std::string DynamicLib::getLastError() const
{
   std::lock_guard<std::mutex> lock(mutex);
   return lastError;
}

void DynamicLib::unloadLocked()
{
   if (!handle) return;
   clearError();

#if defined(_WIN32) || defined(_WIN64)
   if (!::FreeLibrary(static_cast<HMODULE>(handle))) {
      setLastError("FreeLibrary failed: " + getWin32Error());
   }
#else
   if (dlclose(handle) != 0) {
      setLastError(std::string("dlclose failed: ") + dlerror());
   }
#endif
   handle = nullptr;
}

void DynamicLib::clearError()
{
   lastError.clear();
}

void DynamicLib::setLastError(const std::string& err)
{
   lastError = err;
}

#if defined(_WIN32) || defined(_WIN64)
std::string DynamicLib::getWin32Error()
{
   DWORD errCode = GetLastError();
   if (errCode == 0) return "no error";

   LPSTR msgBuf = nullptr;
   size_t size = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, errCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPSTR)&msgBuf, 0, NULL);

   std::string msg(msgBuf, size);
   LocalFree(msgBuf);

   return msg;
}
#endif


} // namespace tbs