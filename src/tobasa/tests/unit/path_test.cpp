#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "tobasa/path.h"

namespace fs = std::filesystem;

namespace {

fs::path makeUniqueTestRoot()
{
   const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
   const auto root = fs::temp_directory_path() / ("tobasa_path_test_" + std::to_string(stamp));
   fs::create_directories(root);
   return root;
}

} // namespace

TEST(TobasaPathTest, CreatesDirectoriesAndTracksFiles)
{
   const auto root = makeUniqueTestRoot();
   const auto nested = root / "nested" / "deep";

   EXPECT_TRUE(tbs::path::createDir(nested.string()));
   EXPECT_TRUE(tbs::path::exists(nested.string()));
   EXPECT_TRUE(tbs::path::isDirectory(nested.string()));

   const auto filePath = nested / "sample.txt";
   const std::string content = "hello path";
   {
      std::ofstream output(filePath);
      output << content;
   }

   EXPECT_TRUE(tbs::path::exists(filePath.string()));
   EXPECT_EQ(tbs::path::fileSize(filePath.string()), content.size());
   EXPECT_EQ(tbs::path::fileNameWithExtension(filePath.string()), "sample.txt");
   EXPECT_EQ(tbs::path::fileExtension("sample.txt"), "txt");
   EXPECT_EQ(tbs::path::getExtension(filePath.string()), "txt");

   std::string errorMessage;
   EXPECT_TRUE(tbs::path::removeFile(filePath.string(), errorMessage));
   EXPECT_FALSE(tbs::path::exists(filePath.string()));

   fs::remove_all(root);
}

TEST(TobasaPathTest, MergesNormalizesAndConvertsPaths)
{
   const auto merged = tbs::path::mergePaths("folder", "file.txt");
   EXPECT_TRUE(merged.find("folder") != std::string::npos);
   EXPECT_TRUE(merged.find("file.txt") != std::string::npos);

   const auto converted = tbs::path::convertToOsPath("folder/sub/file.txt");
   const auto expectedSeparator = std::string(1, tbs::path::SEPARATOR);
   EXPECT_EQ(converted, std::string("folder") + expectedSeparator + "sub" + expectedSeparator + "file.txt");

   const auto normalized = tbs::path::normalize("folder/./sub/../file.txt");
   auto expectedNormalized = fs::path("folder/file.txt").lexically_normal().string();
   EXPECT_EQ(fs::path(normalized).lexically_normal().string(), expectedNormalized);
}

TEST(TobasaPathTest, ChecksSubpathsAndExecutableRelativeResolution)
{
   const auto root = makeUniqueTestRoot();
   const auto base = root / "base";
   const auto child = base / "child";
   const auto sibling = root / "other";

   EXPECT_TRUE(tbs::path::createDir(child.string()));
   EXPECT_TRUE(tbs::path::createDir(sibling.string()));

   EXPECT_TRUE(tbs::path::isPathWithinRoot(child.string(), base.string()));
   EXPECT_FALSE(tbs::path::isPathWithinRoot(sibling.string(), base.string()));

   const auto resolved = tbs::path::resolveExecutableRelativePath(".");
   EXPECT_FALSE(resolved.empty());
   EXPECT_TRUE(fs::path(resolved).is_absolute());

   const auto escaped = tbs::path::resolveExecutableRelativePath("../outside");
   EXPECT_TRUE(escaped.empty());

   fs::remove_all(root);
}
