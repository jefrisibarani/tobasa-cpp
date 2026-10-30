function(check_cmake_project folder version result_var)
   if(IS_DIRECTORY "${folder}" 
      AND EXISTS "${folder}/CMakeLists.txt"
      AND EXISTS "${folder}/${version}.TAG")
      set(${result_var} TRUE PARENT_SCOPE)
   else()
      set(${result_var} FALSE PARENT_SCOPE)
   endif()
endfunction()

macro(set_and_save_cmake_pic_state)
   # Ensure the static library can be linked into a shared library.
   set(_saved_cmake_pic "${CMAKE_POSITION_INDEPENDENT_CODE}")
   set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endmacro()

macro(restore_cmake_pic_state)
   # Restore the previous PIC setting.
   if(DEFINED _saved_cmake_pic AND NOT "${_saved_cmake_pic}" STREQUAL "")
      set(CMAKE_POSITION_INDEPENDENT_CODE "${_saved_cmake_pic}")
   else()
      unset(CMAKE_POSITION_INDEPENDENT_CODE)
   endif()
endmacro()