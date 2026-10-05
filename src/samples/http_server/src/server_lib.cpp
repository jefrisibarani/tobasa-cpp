#include "server_lib.h"
#include <tobasa/datetime.h>
#include <tobasa/path.h>

namespace tbs {
   
bool createUploadFolder(std::filesystem::path& uploadFolder, std::error_code& filesystemError)
{
   namespace fs = std::filesystem;

   fs::path uploadsRoot = fs::path(path::executableDir()) / "app_data" / "uploads";
   fs::create_directories(uploadsRoot, filesystemError);
   if (filesystemError)
      return false;

   auto now = DateTime::now();
   std::string timestamp = now.format("{:%Y%m%d_%H%M%S}") + "_" + std::to_string(now.toUnixTimeMiliSeconds() % 1000);
   for (std::size_t suffix = 0; ; ++suffix)
   {
      std::string folderName = timestamp;
      if (suffix > 0)
         folderName += "_" + std::to_string(suffix);

      uploadFolder = uploadsRoot / folderName;
      if (fs::create_directory(uploadFolder, filesystemError))
         return true;
      if (filesystemError)
         return false;
   }
}

std::string sanitizeUploadFileName(std::string fileName)
{
   auto separator = fileName.find_last_of("/\\");
   if (separator != std::string::npos)
      fileName.erase(0, separator + 1);

   const std::string invalidFileNameChars = "<>:\"/\\|?*";
   for (char& character : fileName)
   {
      if (static_cast<unsigned char>(character) < 32 || invalidFileNameChars.find(character) != std::string::npos)
         character = '_';
   }
   while (!fileName.empty() && (fileName.back() == '.' || fileName.back() == ' '))
      fileName.pop_back();
   if (fileName.empty())
      fileName = "upload";

   return fileName;
}

std::string escapeHtml(const std::string& value)
{
   std::string escaped;
   for (char character : value)
   {
      switch (character)
      {
         case '&':  escaped += "&amp;";  break;
         case '<':  escaped += "&lt;";   break;
         case '>':  escaped += "&gt;";   break;
         case '"': escaped += "&quot;"; break;
         case '\'': escaped += "&#39;";  break;
         default:   escaped += character; break;
      }
   }
   return escaped;
}

std::string encodeUploadPath(const std::filesystem::path& relativePath)
{
   static constexpr char hex[] = "0123456789ABCDEF";
   std::string encoded;
   for (unsigned char character : relativePath.generic_string())
   {
      if ((character >= 'a' && character <= 'z') ||
            (character >= 'A' && character <= 'Z') ||
            (character >= '0' && character <= '9') ||
            character == '-' || character == '_' || character == '.' || character == '~' || character == '/')
      {
         encoded += static_cast<char>(character);
      }
      else
      {
         encoded += '%';
         encoded += hex[character >> 4];
         encoded += hex[character & 0x0F];
      }
   }
   return encoded;
}

  
} // namespace tbs
