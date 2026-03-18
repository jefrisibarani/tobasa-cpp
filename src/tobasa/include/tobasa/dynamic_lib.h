#pragma once

#include <string>
#include <mutex>
#include <stdexcept>
#include <type_traits>   // for std::invoke_result_t

#if defined(_WIN32) || defined(_WIN64)
  #include <windows.h>
#else
  #include <dlfcn.h>
#endif

namespace tbs {

/**
 * @ingroup TBS
 * @class DynamicLib
 * @note Experimental
 * @brief Cross-platform dynamic library loader (Windows: .dll, Linux/macOS: .so/.dylib).
 *
 * Example usage:
 * @code
 * DynamicLib lib("mylib.so");
 * if (!lib.isLoaded()) 
 * {
 *    std::cerr << lib.getLastError() << "\n";
 *    return;
 * }
 *
 * using AddFunc = int(*)(int,int);
 * AddFunc add = lib.requireSymbol<AddFunc>("add");
 * std::cout << "2+3=" << add(2,3) << "\n";
 * @endcode
 */
class DynamicLib
{
public:
   /// Default constructor (library not loaded yet).
   DynamicLib();

   /// Construct and immediately load a library.
   explicit DynamicLib(const std::string& path);

   /// Destructor (unloads library if loaded).
   ~DynamicLib();

   // Non-copyable
   DynamicLib(const DynamicLib&) = delete;
   DynamicLib& operator=(const DynamicLib&) = delete;

   // Movable
   DynamicLib(DynamicLib&& other) noexcept;
   DynamicLib& operator=(DynamicLib&& other) noexcept;

   /**
    * @brief Check if a library is currently loaded.
    * @return true if loaded, false otherwise.
    */
   bool isLoaded() const;

   /**
    * @brief Load a dynamic library by path.
    * @param path Path to .dll (Windows) or .so/.dylib (Linux/macOS).
    * @return true if loaded successfully, false otherwise.
    *
    * Example:
    * @code
    * if (!lib.load("user32.dll")) {
    *     std::cerr << lib.getLastError() << "\n";
    * }
    * @endcode
    */
   bool load(const std::string& path);

   /**
    * @brief Unload the currently loaded library (if any).
    */
   void unload();

   /**
    * @brief Retrieve a raw symbol pointer.
    * @param symbolName Name of the exported symbol.
    * @return pointer to symbol, or nullptr if not found.
    *
    * Example:
    * @code
    * void* sym = lib.getSymbol("someFunction");
    * if (!sym) {
    *     std::cerr << lib.getLastError() << "\n";
    * }
    * @endcode
    */
   void* getSymbol(const std::string& symbolName);

   /**
    * @brief Retrieve and cast a symbol to a function pointer.
    * @tparam T function pointer type.
    * @param symbolName symbol name.
    * @return function pointer or nullptr if not found.
    */
   template <typename T>
   T getFunction(const std::string& symbolName)
   {
      static_assert(std::is_pointer_v<T>, 
         "getFunction<T>: T must be a function pointer type");
      static_assert(std::is_function_v<std::remove_pointer_t<T>>,
         "getFunction<T>: T must be pointer to function");

      void* sym = getSymbol(symbolName);
      if (!sym) 
         return nullptr;

      return reinterpret_cast<T>(sym);
   }

   /**
    * @brief Retrieve and cast a required symbol. Throws if missing.
    * @tparam T function pointer type.
    * @param symbolName symbol name.
    * @return function pointer.
    * @throws std::runtime_error if symbol not found.
    *
    * Example:
    * @code
    * auto add = lib.requireSymbol<int(*)(int,int)>("add");
    * std::cout << add(1,2);
    * @endcode
    */
   template <typename T>
   T requireSymbol(const std::string& symbolName)
   {
      void* sym = getSymbol(symbolName);
      if (!sym) {
         throw std::runtime_error(
               "Required symbol '" + symbolName + "' not found. Last error: " +
               getLastError());
      }
      return reinterpret_cast<T>(sym);
   }

   /**
    * @brief Get last error message (from load/getSymbol/unload).
    * @return error string.
    */
   std::string getLastError() const;

private:
   void* handle;
   mutable std::mutex mutex;
   std::string lastError;

   void unloadLocked();
   void clearError();
   void setLastError(const std::string& err);

#if defined(_WIN32) || defined(_WIN64)
   static std::string getWin32Error();
#endif
};


// -------------------------------------------------------
// Function pointer *type* macros
// -------------------------------------------------------
// Example:
//   using MsgBoxType = DYN_FUNC_T(int, HWND, LPCSTR, LPCSTR, UINT);
// Expands (on Windows) to:
//   using MsgBoxType = int (WINAPI *)(HWND, LPCSTR, LPCSTR, UINT);
// Then we can use with DynamicSymbol:
//   DynamicSymbol<MsgBoxType> msgBox(lib, "MessageBoxA");
//
#if defined(_WIN32) || defined(_WIN64)
   #define DYN_STDCALL_T(ret, ...) ret (__stdcall *)(__VA_ARGS__)
   #define DYN_CDECL_T(ret, ...)   ret (__cdecl   *)(__VA_ARGS__)
   #define DYN_FUNC_T(ret, ...)    ret (WINAPI    *)(__VA_ARGS__)  // WinAPI standard
#else
   #define DYN_STDCALL_T(ret, ...) ret (*)(__VA_ARGS__)
   #define DYN_CDECL_T(ret, ...)   ret (*)(__VA_ARGS__)
   #define DYN_FUNC_T(ret, ...)    ret (*)(__VA_ARGS__)
#endif

// -------------------------------------------------------
// Function pointer *declaration* macros
// (if you want to declare function pointer variables directly)
// -------------------------------------------------------
// Example:
//    DYN_FUNC(int, msgBox, HWND, LPCSTR, LPCSTR, UINT);
// Expands (on Windows) to:
//    int (WINAPI *msgBox)(HWND, LPCSTR, LPCSTR, UINT);
// Then we can use with getFunction():
//    msgBox = lib.getFunction<decltype(msgBox)>("MessageBoxA");
//
#if defined(_WIN32) || defined(_WIN64)
   #define DYN_STDCALL(ret, name, ...) ret (__stdcall *name)(__VA_ARGS__)
   #define DYN_CDECL(ret, name, ...)   ret (__cdecl   *name)(__VA_ARGS__)
   #define DYN_FUNC(ret, name, ...)    ret (WINAPI    *name)(__VA_ARGS__)
#else
   #define DYN_STDCALL(ret, name, ...) ret (*name)(__VA_ARGS__)
   #define DYN_CDECL(ret, name, ...)   ret (*name)(__VA_ARGS__)
   #define DYN_FUNC(ret, name, ...)    ret (*name)(__VA_ARGS__)
#endif


/**
 * @class DynamicSymbol
 * @brief RAII wrapper for a function symbol loaded from DynamicLib.
 *
 * Example:
 * @code
 * 
 * # With type-only macros:
 * 
 * DynamicSymbol<DYN_FUNC_T(int, HWND, LPCSTR, LPCSTR, UINT)> msgBox(lib, "MessageBoxA");
 * if (msgBox) {
 *    msgBox(NULL, "Hello", "Title", MB_OK);
 * }
 * 
 * # With a using alias (cleaner):
 * 
 * using MsgBoxType = DYN_FUNC_T(int, HWND, LPCSTR, LPCSTR, UINT);
 * DynamicSymbol<MsgBoxType> msgBox(lib, "MessageBoxA");
 * 
 * @endcode
 */
template <typename T>
class DynamicSymbol
{
public:
   DynamicSymbol() : _func(nullptr), _lib(nullptr) {}
   DynamicSymbol(DynamicLib& library, const std::string& symbolName)
   {
      load(library, symbolName);
   }

   /// Load a symbol from library.
   bool load(DynamicLib& library, const std::string& symbolName)
   {
      _lib = &library;
      _func = library.getFunction<T>(symbolName); 
      return _func != nullptr;
   }

   /// Check if symbol is valid.
   bool isValid() const { return _func != nullptr; }

   /// Invoke like a normal function pointer.
   // Note: two-phase lookup in templates. we explicitly use this->
   template <typename... Args>
   //auto operator()(Args... args) const -> decltype(this->_func(args...))
   auto operator()(Args... args) const -> std::invoke_result_t<T, Args...>
   {
      if (!this->_func)
      {
         throw std::runtime_error(
            "Attempted to call invalid DynamicSymbol. Last error: " +
            (_lib ? _lib->getLastError() : std::string("no library")));
      }
      return this->_func(args...);
   }

   explicit operator bool() const noexcept { return this->_func != nullptr; }

   T get() const { return this->_func; }

private:
   T _func;
   DynamicLib* _lib; // for error reporting only
};

} // namespace tbs