#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "comet::comet" for configuration "Debug"
set_property(TARGET comet::comet APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(comet::comet PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libcomet.so"
  IMPORTED_SONAME_DEBUG "libcomet.so"
  )

list(APPEND _cmake_import_check_targets comet::comet )
list(APPEND _cmake_import_check_files_for_comet::comet "${_IMPORT_PREFIX}/lib/libcomet.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
