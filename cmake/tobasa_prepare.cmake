# Check for std::format and std::date
# -------------------------------------------------------
if(MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.30.30705)
   set(TOBASA_USE_STD_TEXT_FORMATTING TRUE CACHE INTERNAL "" ) # __cpp_lib_format >= 201907L (Text formatting)
   set(TOBASA_USE_STD_CALENDAR_TZONE  TRUE CACHE INTERNAL "" ) # __cpp_lib_chrono >= 201803L (Calendar and Time Zone library) 
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14.0)
   set(TOBASA_USE_STD_TEXT_FORMATTING TRUE CACHE INTERNAL "" )
   set(TOBASA_USE_STD_CALENDAR_TZONE  TRUE CACHE INTERNAL "" )
else()
  set(TOBASA_USE_STD_CALENDAR_TZONE FALSE CACHE INTERNAL "" )
  set(TOBASA_USE_STD_TEXT_FORMATTING FALSE CACHE INTERNAL "" )
endif()
set(TOBASA_STD_FORMAT_AND_DATE_CHECKED TRUE CACHE INTERNAL "" )


# Support for c++ 11, 14 17
# -------------------------------------------------------
#taken from dcmtkPrepare.cmake
define_property(GLOBAL PROPERTY TOBASA_MODERN_CXX_STANDARDS
  BRIEF_DOCS "Modern C++ standards Tobasa knows about."
  FULL_DOCS "The list of C++ standards since C++11 that Tobasa currently has configuration tests for. "
)
set_property(GLOBAL PROPERTY TOBASA_MODERN_CXX_STANDARDS 11 14 17)

set(TOBASA_HAVE_CXX11 1 CACHE INTERNAL "Set to 1 if the compiler supports C++${STANDARD} and it should be enabled.")
set(TOBASA_HAVE_CXX14 1 CACHE INTERNAL "Set to 1 if the compiler supports C++${STANDARD} and it should be enabled.")
set(TOBASA_HAVE_CXX17 1 CACHE INTERNAL "Set to 1 if the compiler supports C++${STANDARD} and it should be enabled.")


# Enable /Zc:__cplusplus option for Visual Studio 2017 and above
# -------------------------------------------------------
# Note: taken from GenerateDCMTKConfigure.cmake
# Visual Studio >= 2017 supports C++11 and later, but does not set
# the __cplusplus macro with the supported C++ standard version.
# /Zc:__cplusplus will enforce setting this macro in Visual Studio.
# VS Versions < 2017 do not support this switch.
# See also https://learn.microsoft.com/de-de/cpp/build/reference/zc-cplusplus
set(FORCE_MSVC_CPLUSPLUS_MACRO "")
if(MSVC)
  if(NOT (MSVC_VERSION LESS 1910)) # VS 2017 and above
    set (FORCE_MSVC_CPLUSPLUS_MACRO "/Zc:__cplusplus")
  endif()
endif()

# if at least one modern C++ standard should be supported,
# add FORCE_MSVC_CPLUSPLUS_MACRO for MSVC to enforce setting
# of __cplusplus macro
if(MSVC)
  get_property(MODERN_CXX_STANDARDS GLOBAL PROPERTY TOBASA_MODERN_CXX_STANDARDS)
  foreach(STANDARD ${MODERN_CXX_STANDARDS})
    if(TOBASA_HAVE_CXX${STANDARD})
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FORCE_MSVC_CPLUSPLUS_MACRO}")
      break()
    endif()
  endforeach()
endif()


# Set global build output files 
# -------------------------------------------------------
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")


# File to C Array generator
# -------------------------------------------------------
add_executable(generate_resources_file ${PROJECT_SOURCE_DIR}/cmake/generate_resources.cpp)
if(MSVC)
  set_property(TARGET generate_resources_file PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
target_compile_features(generate_resources_file PUBLIC cxx_std_17)


# Time Zone Database to C Array generator
# -------------------------------------------------------
add_executable(generate_tzdata_assets_file ${PROJECT_SOURCE_DIR}/cmake/generate_tzdata_assets.cpp)
if(MSVC)
  set_property(TARGET generate_tzdata_assets_file PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
target_compile_features(generate_tzdata_assets_file PUBLIC cxx_std_17)


# Build Info generator
# -------------------------------------------------------
add_executable(generate_build_info ${PROJECT_SOURCE_DIR}/cmake/generate_build_info.cpp)
if(MSVC)
  set_property(TARGET generate_build_info PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
target_compile_features(generate_build_info PUBLIC cxx_std_17)