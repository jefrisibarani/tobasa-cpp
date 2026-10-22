#pragma once

#include <string>
#include "tobasa/exp_imp.h"

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
   TBS_API std::string executablePath();
   
   /// Get executable absolute directory.
   TBS_API std::string executableDir();

   /// Get system temporary directory
   TBS_API std::string temporaryDir();

   /// Get Application data directory
   TBS_API std::string appdataDir();

   TBS_API std::string homeDir();

   /// Merge two paths.
   TBS_API std::string mergePaths(std::string path1, std::string path2);

   /// Check for an existing path.
   TBS_API bool exists(const std::string& path);
   
   /// Get filename with extension
   TBS_API std::string fileNameWithExtension(const std::string& fileFullPath);

   /// Get file extension part.
   TBS_API std::string fileExtension(const std::string& fileName);
   
   /// Get file extension from full path
   TBS_API std::string getExtension(const std::string& path);

   /// Check if a path is a directory.
   TBS_API bool isDirectory(const std::string& path);
   
   /// Create a directory.
   TBS_API bool mkdir(const std::string& path);
   
   /// @brief Create the given directory - and all directories leading to it
   /// @return true on success or if the directory already exists
   TBS_API bool createDir(const std::string& path);

   /// Get the size of a file in bytes.
   TBS_API size_t fileSize(const std::string& path);

   /// Convert a path to an absolute path.
   TBS_API std::string absolute(const std::string& path);

   /// Remove a file and return an error message on failure.
   TBS_API bool removeFile(const std::string& path, std::string& errorMessage);

   /// Check weheter a path is located within the given rootPath
   TBS_API bool isPathWithinRoot(const std::string& candidatePath, const std::string& rootPath);

   /// Convert a path to the operating system's path format.
   TBS_API std::string convertToOsPath(const std::string& path);

   /// Normalize a path.
   TBS_API std::string normalize(const std::string& path);
   
   /// Resolves a path relative to the executable directory.
   /// Absolute paths are returned as-is.
   /// Returns empty string if the path escapes the executable directory.
   TBS_API std::string resolveExecutableRelativePath(const std::string& path);

} // namespace path
} // namespace tbs