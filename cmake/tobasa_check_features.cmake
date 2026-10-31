
include_guard(GLOBAL)

include(CheckCXXSourceCompiles)

# Check support for floating-point std::to_chars().
function(tobasa_check_float_to_chars)
   check_cxx_source_compiles("
      #include <charconv>
      #include <system_error>

      int main()
      {
         char buffer[64];
         double value = 1.25;
         auto result = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::general);
         return result.ec == std::errc() ? 0 : 1;
      }
   " TOBASA_HAS_FLOAT_TO_CHARS)

   if(TOBASA_HAS_FLOAT_TO_CHARS)
      message(STATUS "tobasa Floating-point std::to_chars(): supported")
   else()
      message(STATUS "tobasa Floating-point std::to_chars(): unavailable")
   endif()
endfunction()


# Check support for std::chrono::parse().
function(tobasa_check_std_chrono_parse)
   check_cxx_source_compiles("
      #include <chrono>
      #include <sstream>

      int main()
      {
         std::istringstream input(\"2024-01-01\");
         std::chrono::year_month_day value;
         input >> std::chrono::parse(\"%F\", value);
         return input.fail();
      }
   " TOBASA_HAS_STD_CHRONO_PARSE)

   if(TOBASA_HAS_STD_CHRONO_PARSE)
      message(STATUS "tobasa std::chrono::parse(): supported")
   else()
      message(STATUS "tobasa std::chrono::parse(): unavailable")
   endif()
endfunction()
