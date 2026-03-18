#pragma once

#include <string>

namespace tbs {
namespace path {

/** 
 * @ingroup TBS
 * @brief Path utilites
 */

#ifdef _WIN32
   constexpr const char SEPARATOR = '\\';
#else   // !_WIN32
   constexpr const char SEPARATOR = '/';
#endif  // !_WIN32
   
   /// Get executable absolute path.
   std::string executablePath();
   
   /// Get executable absolute directory.
   std::string executableDir();

   /// Get system temporary directory
   std::string temporaryDir();

   /// Get Application data directory
   std::string appdataDir();

   std::string homeDir();

   /// Merge two paths.
   std::string mergePaths(std::string path1, std::string path2);

   /// Check for an existing path.
   bool exists(const std::string& path);
   
   /// Get filename with extension
   std::string fileNameWithExtension(const std::string& fileFullPath);

   /// Get file extension part.
   std::string fileExtension(const std::string& fileName);
   
   /// Get file extension from full path
   std::string getExtension(const std::string& path);

   /// Check if a path is a directory.
   bool isDirectory(const std::string& path);
   
   /// Create a directory.
   bool mkdir(const std::string& path);
   
   /// @brief Create the given directory - and all directories leading to it
   /// @return true on success or if the directory already exists
   bool createDir(const std::string& path);
   
   /// Get file size.
   /// @param path file path
   size_t fileSize(const std::string& path);

   /// Get absolute path.
   std::string absolute(const std::string& path);

   /// Remove file.
   bool removeFile(const std::string& path, std::string& errorMessage);

   bool isSubPath(const std::string& path, const std::string& base);

   /// 
   std::string convertToOsPath(const std::string& path);

   std::string normalize(const std::string& path);
   
   /// Resolves a path relative to the executable directory.
   /// Absolute paths are returned as-is.
   /// Returns empty string if the path escapes the executable directory.
   std::string resolveExecutableRelativePath(const std::string& path);

} // namespace path
} // namespace tbs