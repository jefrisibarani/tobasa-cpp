#pragma once

#include <filesystem>
#include <string>
#include <system_error>

namespace tbs {

bool createUploadFolder(std::filesystem::path& uploadFolder, std::error_code& filesystemError);

std::string sanitizeUploadFileName(std::string fileName);

std::string escapeHtml(const std::string& value);

std::string encodeUploadPath(const std::filesystem::path& relativePath);

} // namespace tbs
