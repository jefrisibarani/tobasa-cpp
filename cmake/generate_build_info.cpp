/*
   File     : generate_build_info
   Author   : Jefri Sibarani
*/

#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

// Detect compiler ID
std::string detectCompilerId() {
#if defined(_MSC_VER)
   return "MSVC";
#elif defined(__clang__)
   return "Clang";
#elif defined(__GNUC__)
   return "GCC";
#else
   return "Unknown";
#endif
}

// Detect compiler version
std::string detectCompilerVersion() 
{
#if defined(_MSC_VER)
   return std::to_string(_MSC_VER);
#elif defined(__clang__)
   return std::to_string(__clang_major__) + "." +
          std::to_string(__clang_minor__) + "." +
          std::to_string(__clang_patchlevel__);
#elif defined(__GNUC__)
   return std::to_string(__GNUC__) + "." +
          std::to_string(__GNUC_MINOR__) + "." +
          std::to_string(__GNUC_PATCHLEVEL__);
#else
   return "Unknown";
#endif
}

// Detect platform
std::string detectPlatform() {
#if defined(_WIN64)
   return "x64";
#elif defined(_WIN32)
   return "x86";
#elif defined(__APPLE__)
   return "macOS";
#elif defined(__linux__)
   return "Linux";
#elif defined(__aarch64__)
   return "ARM64";
#elif defined(__arm__)
   return "ARM";
#else
   return "Unknown";
#endif
}

// Try to find VERSION file in current or parent directories
std::string detectAppVersion(const fs::path& startDir) 
{
   fs::path current = startDir;
   for (int i = 0; i < 5; ++i) 
   {  
      // search up to 5 levels up
      fs::path versionFile = current / "VERSION";
      if (fs::exists(versionFile)) 
      {
         std::ifstream in(versionFile);
         std::string version;
         std::getline(in, version);
         if (!version.empty())
            return version;
      }
      if (current.has_parent_path())
         current = current.parent_path();
      else
         break;
   }
   return "1.0.0";
}

int main(int argc, char* argv[]) 
{
   // Example usage:
   //   generate_build_info.exe [outputCppFile] [compilerId] [compilerVer] [compilerPlatform]

   //std::string cmdPath = argv[0];

   // Detect values
   std::string targetFilePath   = (argc > 1) ? argv[1] : detectCompilerId();
   std::string compilerId       = (argc > 2) ? argv[2] : detectCompilerId();
   std::string compilerVer      = (argc > 3) ? argv[3] : detectCompilerVersion();
   std::string compilerPlatform = (argc > 4) ? argv[4] : detectPlatform();

   fs::path targetPath = targetFilePath; //fs::path(targetFilePath);
   fs::path targetPathDir = fs::path(targetFilePath).parent_path();

   std::string versionId = detectAppVersion(targetPathDir);

   // Build UTC date/time
   auto now = std::chrono::system_clock::now();
   std::time_t now_c = std::chrono::system_clock::to_time_t(now);
   std::tm utc_tm{};
#if defined(_WIN32)
   gmtime_s(&utc_tm, &now_c);
#else
   gmtime_r(&now_c, &utc_tm);
#endif

   std::ostringstream dateStream;
   dateStream << std::put_time(&utc_tm, "%Y-%m-%d %H:%M:%S UTC");
   std::string buildDate = dateStream.str();

   // Output file content
   std::ostringstream content;
   content <<
R"(#include "build_info.h"

namespace tbs
{
   std::string appVersion()       { return ")" << versionId << R"("; }
   std::string appBuildDate()     { return ")" << buildDate << R"("; }
   std::string compilerId()       { return ")" << compilerId << R"("; }
   std::string compilerVersion()  { return ")" << compilerVer << R"("; }
   std::string compilerPlatform() { return ")" << compilerPlatform << R"("; }
}
)";

   // Write file
   std::ofstream out(targetPath);
   if (!out) 
   {
      std::cerr << "Failed to write: " << targetPath << "\n";
      return 1;
   }
   out << content.str();
   out.close();

   std::cout << "Generated: " << targetPath << "\n";
   std::cout << "   Version:  " << versionId << "\n";
   std::cout << "   Compiler: " << compilerId << " " << compilerVer
            << " (" << compilerPlatform << ")\n";
   std::cout << "   Build:    " << buildDate << "\n";

   return 0;
}