#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "netlib::bbnetlib" for configuration "Debug"
set_property(TARGET netlib::bbnetlib APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(netlib::bbnetlib PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libbbnetlibd.a"
  )

list(APPEND _cmake_import_check_targets netlib::bbnetlib )
list(APPEND _cmake_import_check_files_for_netlib::bbnetlib "${_IMPORT_PREFIX}/lib/libbbnetlibd.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
