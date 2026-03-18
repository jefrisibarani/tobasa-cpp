#if defined(_WIN32)
   #include <windows.h>
   #include <shlwapi.h>
   #include <io.h>
   #include <direct.h> // for _mkdir/_wmkdir
   #include <shlobj.h> // for getHomeDir()
#endif

#ifdef __APPLE__
   #include <libgen.h>
   #include <limits.h>
   #include <mach-o/dyld.h>
   #include <unistd.h>
   #include <pwd.h> // for getHomeDir()
#endif

#ifdef __linux__
   #include <stdio.h>
   #include <sys/stat.h>
   #include <sys/types.h>
   #include <limits.h>
   #include <libgen.h>
   #include <unistd.h>
   #include <cstring>
   #include <pwd.h> // for getHomeDir()
   
   #if defined(__sun)
      #define PROC_SELF_EXE "/proc/self/path/a.out"
   #else
      #define PROC_SELF_EXE "/proc/self/exe"
   #endif
#endif

#include <string>
#include <sstream>
#include <filesystem>
#include "tobasa/exception.h"
#include "tobasa/path.h"

namespace tbs {
namespace path {

#if defined(_WIN32)

std::string executablePath()
{
   char rawPathName[MAX_PATH];
   GetModuleFileNameA(NULL, rawPathName, MAX_PATH);
   return std::string(rawPathName);
}

std::string executableDir()
{
   std::string p = path::executablePath();
   char* exePath = new char[p.length()+1];
   strcpy_s(exePath, p.length()+1, p.c_str());
   PathRemoveFileSpecA(exePath);
   std::string exeDir = std::string(exePath);
   delete[] exePath;
   return exeDir;
}

std::string mergePaths(std::string path1, std::string path2)
{
   char combined[MAX_PATH];
   PathCombineA(combined, path1.c_str(), path2.c_str());
   std::string mergedPath(combined);
   return mergedPath;
}

#endif // defined(_WIN32)


#ifdef __linux__

std::string executablePath()
{
   char rawPathName[PATH_MAX];
   realpath(PROC_SELF_EXE, rawPathName);
   return  std::string(rawPathName);
}

std::string executableDir()
{
   std::string p = executablePath();
   char *pstr = new char[p.length() + 1];
   strcpy(pstr, p.c_str());
   char* exeDir = dirname(pstr);
   std::string result{exeDir};
   delete [] pstr;
   return result;
}

std::string mergePaths(std::string path1, std::string path2)
{
   return path1 + "/" + path2;
}

#endif // __linux__


#ifdef __APPLE__

std::string executablePath()
{
   char rawPathName[PATH_MAX];
   char realPathName[PATH_MAX];
   uint32_t rawPathSize = (uint32_t)sizeof(rawPathName);

   if(!_NSGetExecutablePath(rawPathName, &rawPathSize)) {
      realpath(rawPathName, realPathName);
   }
   return std::string(realPathName);
}

std::string executableDir()
{
   std::string p = path::executablePath();
   char *pstr = new char[p.length() + 1];
   strcpy(pstr, p.c_str());
   char* exeDir = dirname(pstr);
   std::string result{exeDir};
   delete [] pstr;
   return result;
}

std::string mergePaths(std::string path1, std::string path2)
{
   return path1 + "/" + path2;
}

#endif // __APPLE__



std::string temporaryDir() 
{
#if defined(_WIN32)
   char buffer[MAX_PATH];
   DWORD len = GetTempPathA(MAX_PATH, buffer);
   if (len > 0 && len < MAX_PATH) {
      return std::string(buffer);
   } else {
      return "C:\\Temp"; // fallback
   }
#else
   const char* tmp = std::getenv("TMPDIR");
   if (tmp && *tmp) {
      return std::string(tmp);
   }
   return "/tmp"; // default fallback on Unix-like systems
#endif
}


std::string appdataDir()
{
#if defined(_WIN32)
   // On Windows, prefer APPDATA (e.g. C:\Users\<User>\AppData\Roaming)
   const char* appdata = std::getenv("APPDATA");
   if (appdata && *appdata)
      return std::string(appdata);

   // Fallback to USERPROFILE if APPDATA not set
   const char* userprofile = std::getenv("USERPROFILE");
   if (userprofile && *userprofile)
      return std::filesystem::path(userprofile).append("AppData\\Roaming").string();

   return "C:\\Users\\Default\\AppData\\Roaming"; // last fallback

#else
   // On Linux/macOS, use $XDG_CONFIG_HOME or $HOME/.config
   const char* xdg = std::getenv("XDG_CONFIG_HOME");
   if (xdg && *xdg)
      return std::string(xdg);

   const char* home = std::getenv("HOME");
   if (home && *home)
      return std::filesystem::path(home).append(".config").string();

   return "/tmp"; // fallback if HOME not set
#endif
}


std::string homeDir()
{
#ifdef _WIN32
   PWSTR widePath = nullptr;
   std::string result;

   if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &widePath)))
   {
      // Convert UTF-16 → UTF-8
      int size = WideCharToMultiByte(
         CP_UTF8,
         0,
         widePath,
         -1,
         nullptr,
         0,
         nullptr,
         nullptr
      );

      if (size > 0)
      {
         result.resize(size - 1);
         WideCharToMultiByte(
            CP_UTF8,
            0,
            widePath,
            -1,
            &result[0],
            size,
            nullptr,
            nullptr
         );
      }
   }

   if (widePath)
      CoTaskMemFree(widePath);

   return result;

#else
   // POSIX (Linux, macOS, BSD)
   const char* home = getenv("HOME");
   if (home && *home)
      return std::string(home);

   // Fallback: passwd database
   struct passwd* pw = getpwuid(getuid());
   if (pw && pw->pw_dir)
      return std::string(pw->pw_dir);

   return "";
#endif
}

bool exists(const std::string& path)
{
#ifdef _WIN32
   return ::_access(path.c_str(), 0) == 0;
#else
   return ::access(path.c_str(), F_OK) == 0;
#endif
}

std::string fileNameWithExtension(const std::string& fileFullPath)
{
   namespace fs = std::filesystem;
   fs::path filePath(fileFullPath);
    
   // Extract filename with extension
   std::string filename = filePath.filename().string();
   return filename;
}


std::string fileExtension(const std::string& fileName)
{
   size_t lastDotPos  = fileName.find_last_of(".");
   if (lastDotPos != std::string::npos) 
   {
      std::string ext = fileName.substr(lastDotPos + 1);
      return ext;
   }

   return std::string();
}

/// Get file extension from full path
std::string getExtension(const std::string& path)
{
   std::size_t lastSlashPos = path.find_last_of("/");
   std::size_t lastDotPos   = path.find_last_of(".");
   if (lastDotPos != std::string::npos && lastDotPos > lastSlashPos) 
   {
      std::string ext = path.substr(lastDotPos + 1);
      return ext;
   }

   return std::string();
}

bool isDirectory(const std::string& path)
{
   struct stat s;
   if ( stat(path.c_str(),&s) == 0 )
   {
      if ( s.st_mode & S_IFDIR )
         return true;
      else if( s.st_mode & S_IFREG ) {}//it's a file
      else {}//something else
   }
   else {} // error

   return false;
}

bool mkdir(const std::string& path)
{
#ifdef _WIN32
   return ::_mkdir(path.c_str()) == 0;
#else
   return ::mkdir(path.c_str(), mode_t(0755)) == 0;
#endif
}


// note: from spdlog os-inl.h
bool createDir(const std::string& path)
{
   if (path::exists(path)) 
      return true;

   if (path.empty())
      return false;

   size_t searchOffset = 0;
   do
   {
      auto token_pos = path.find_first_of(path::SEPARATOR, searchOffset);
      // treat the entire path as a folder if no folder separator not found
      if (token_pos == std::string::npos) {
         token_pos = path.size();
      }

      auto subdir = path.substr(0, token_pos);

      if (!subdir.empty() && !path::exists(subdir) && !path::mkdir(subdir))
         return false; // return error if failed creating dir

      searchOffset = token_pos + 1;

   } while (searchOffset < path.size());

   return true;
}

size_t fileSize(const std::string& path)
{
   namespace fs = std::filesystem;
   fs::path p = path;
   std::error_code ec;   
   size_t size = fs::file_size(p, ec);

   if (ec)
      throw tbs::AppException(ec.message());

   return size;
}

std::string absolute(const std::string& path)
{
   namespace fs = std::filesystem;
   fs::path p = path;
   return fs::absolute(p).string();
}

bool removeFile(const std::string& path, std::string& errorMessage)
{
   namespace fs = std::filesystem;
   std::error_code ec;
   fs::path p{path};
   if (fs::remove(p, ec)) 
      return true;
   else
   {
      errorMessage = ec.message();
      return false;      
   }
}

bool isSubPath(const std::string& path, const std::string& base)
{
   namespace fs = std::filesystem;
   fs::path _path = path;
   fs::path _base = base;

   auto p = _path.lexically_normal();
   auto b = _base.lexically_normal();

   auto mismatch = std::mismatch(
      b.begin(), b.end(),
      p.begin(), p.end()
   );

   // If we reached the end of base, then base is a prefix of path
   return mismatch.first == b.end();
}

std::string convertToOsPath(const std::string& path)
{
   std::string fullPath(path);
#ifdef _WIN32
   std::replace(fullPath.begin(), fullPath.end(), '/', '\\');
#else
   std::replace(fullPath.begin(), fullPath.end(), '\\', '/');
#endif
   return fullPath;
}

std::string normalize(const std::string& path)
{
   std::filesystem::path p(path);
   p = p.lexically_normal();
   return p.string();
/*
   if (path.empty())
      return "";

   std::string cleanPath = path;
#if defined(_WIN32)
   std::replace(cleanPath.begin(), cleanPath.end(), '/', '\\');
   const char pathDelimiter = '\\';
#else
   std::replace(cleanPath.begin(), cleanPath.end(), '\\', '/');
   const char pathDelimiter = '/';
#endif

   std::stringstream ss(cleanPath);
   std::string token;
   std::vector<std::string> components;

   // Detect if path is absolute (starts with slash or drive letter)
   bool isAbsolute = false;
#if defined(_WIN32)
   if (cleanPath.size() > 1 && cleanPath[1] == ':')
      isAbsolute = true;
#else
   if (!cleanPath.empty() && cleanPath[0] == '/')
      isAbsolute = true;
#endif

   while (std::getline(ss, token, pathDelimiter)) {
      if (token == "..") {
         if (!components.empty())
               components.pop_back();
      } else if (token != "." && !token.empty()) {
         components.push_back(token);
      }
   }

   std::ostringstream normalized;
   if (isAbsolute) {
#if defined(_WIN32)
      if (cleanPath.size() > 1 && cleanPath[1] == ':')
         normalized << cleanPath.substr(0, 2); // preserve drive, e.g. "C:"
#endif
      normalized << pathDelimiter;
   }

   for (size_t i = 0; i < components.size(); ++i) {
      if (i > 0)
         normalized << pathDelimiter;
      normalized << components[i];
   }

   return normalized.str();
*/   
}

std::string resolveExecutableRelativePath(const std::string& path)
{
   namespace fs = std::filesystem;
   fs::path p = path;
   if (p.is_absolute()) {
      return p.make_preferred().string();
   }

   // Resolve relative to executable directory
   fs::path base = path::executableDir();
   fs::path full = fs::weakly_canonical(base / p);
   if (!path::isSubPath(full.string(), base.string()))
   {
      return "";
   }
   
   return full.make_preferred().string();
}

} // namespace path
} // namespace tbs