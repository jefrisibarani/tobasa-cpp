#include <gtest/gtest.h>

#include "tobasa/util_utf.h"

TEST(TobasaUtilUtfTest, ValidatesUtf8Strings)
{
   EXPECT_TRUE(tbs::util::isValidUtf8("hello"));
   EXPECT_TRUE(tbs::util::isValidUtf8("café 東京"));
   EXPECT_FALSE(tbs::util::isValidUtf8(std::string("\xFF", 1)));
}

TEST(TobasaUtilUtfTest, ConvertsUtf8ToWideString)
{
   const std::string utf8 = std::string("caf") + std::string("\xC3\xA9", 2) + " " + std::string("\xE6\x9D\xB1", 3) + std::string("\xE4\xBA\xAC", 3);
   const std::wstring wide = tbs::util::utf8_to_wstring(utf8);

   std::wstring expected = L"caf";
   expected.push_back(static_cast<wchar_t>(0x00E9));
   expected += L" ";
   expected.push_back(static_cast<wchar_t>(0x6771));
   expected.push_back(static_cast<wchar_t>(0x4EAC));

   EXPECT_EQ(wide, expected);
}

TEST(TobasaUtilUtfTest, ConvertsWideStringToUtf8)
{
   std::wstring wide = L"caf";
   wide.push_back(static_cast<wchar_t>(0x00E9));
   wide += L" ";
   wide.push_back(static_cast<wchar_t>(0x6771));
   wide.push_back(static_cast<wchar_t>(0x4EAC));

   const std::string utf8 = tbs::util::wstring_to_utf8(wide);
   const std::string expected = std::string("caf") + std::string("\xC3\xA9", 2) + " " + std::string("\xE6\x9D\xB1", 3) + std::string("\xE4\xBA\xAC", 3);

   EXPECT_EQ(utf8, expected);
}

TEST(TobasaUtilUtfTest, RoundTripsUtf8AndWideStrings)
{
   const std::string utf8 = std::string("A") + std::string("\xC3\xA9", 2) + std::string("\xE6\x9D\xB1", 3) + std::string("\xE4\xBA\xAC", 3);
   const auto wide = tbs::util::utf8_to_wstring(utf8);
   const auto roundTrip = tbs::util::wstring_to_utf8(wide);

   EXPECT_EQ(roundTrip, utf8);
   EXPECT_TRUE(tbs::util::isValidUtf8(roundTrip));
}
