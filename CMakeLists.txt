cmake_minimum_required(VERSION 2.8.12)
project (python-moos)
find_package(MOOS 10 REQUIRED)

add_subdirectory(pybind11)
pybind11_add_module(pymoos pyMOOS.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${MOOS_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIR})

set_target_properties(pymoos
  PROPERTIES
  SUFFIX ".so")

set_target_properties(pymoos
  PROPERTIES
  PREFIX "")

#########################################################Saad
if (WIN32)
  if (MSVC)
    # /bigobj is needed for bigger binding projects due to the limit to 64k
    # addressable sections. /MP enables multithreaded builds (relevant when
    # there are many files).
    set_target_properties(pymoos PROPERTIES COMPILE_FLAGS "/MP /bigobj ")

    if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
      # Enforce size-based optimization and link time code generation on MSVC
      # (~30% smaller binaries in experiments).
      set_target_properties(pymoos APPEND_STRING PROPERTY COMPILE_FLAGS "/Os /GL ")
      set_target_properties(pymoos APPEND_STRING PROPERTY LINK_FLAGS "/LTCG ")
    endif()
  endif()

  # .PYD file extension on Windows
  set_target_properties(pymoos PROPERTIES SUFFIX ".pyd")

  # Link against the Python shared library
  target_link_libraries(pymoos ${PYTHON_LIBRARY} ${MOOS_LIBRARIES})
elseif (UNIX)
  # It's quite common to have multiple copies of the same Python version
  # installed on one's system. E.g.: one copy from the OS and another copy
  # that's statically linked into an application like Blender or Maya.
  # If we link our plugin library against the OS Python here and import it
  # into Blender or Maya later on, this will cause segfaults when multiple
  # conflicting Python instances are active at the same time (even when they
  # are of the same version).

  # Windows is not affected by this issue since it handles DLL imports
  # differently. The solution for Linux and Mac OS is simple: we just don't
  # link against the Python library. The resulting shared library will have
  # missing symbols, but that's perfectly fine -- they will be resolved at
  # import time.

  # .SO file extension on Linux/Mac OS
  set_target_properties(pymoos PROPERTIES SUFFIX ".so")

  # Strip unnecessary sections of the binary on Linux/Mac OS
  if(APPLE)
    set_target_properties(pymoos PROPERTIES MACOSX_RPATH ".")
    set_target_properties(pymoos PROPERTIES LINK_FLAGS "-undefined dynamic_lookup ")
    if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
      add_custom_command(TARGET pymoos POST_BUILD COMMAND strip -u -r ${PROJECT_BINARY_DIR}/pymoos.so)
    endif()
  else()
    if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
      add_custom_command(TARGET pymoos POST_BUILD COMMAND strip ${PROJECT_BINARY_DIR}/pymoos.so)
    endif()
  endif()

  target_link_libraries(pymoos ${PYTHON_LIBRARIES} ${MOOS_LIBRARIES})
endif()

file(GLOB ExampleFiles ${CMAKE_SOURCE_DIR}/Documentation/examples/*.py)
add_custom_target(copy)
get_target_property(pymoosLocation pymoos LOCATION)
get_filename_component(pymoosDir ${pymoosLocation} PATH)
foreach(ExampleFile ${ExampleFiles})
  add_custom_command(TARGET copy PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E
                         copy ${ExampleFile} ${pymoosDir})
endforeach()
add_dependencies(pymoos copy)
