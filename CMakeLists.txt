cmake_minimum_required(VERSION 2.6)
project (python-moos)
find_package(MOOS 10 REQUIRED)

find_package(PythonLibs)
if(PYTHONLIBS_FOUND)
  include_directories("${PYTHON_INCLUDE_DIRS}")
else()
  message(FATAL_ERROR "Unable to find PythonLibs.")
endif()

find_package(Boost REQUIRED python)
set(Boost_USE_STATIC_LIBS     ON)
set(Boost_USE_MULTITHREADED    ON)
set(Boost_USE_STATIC_RUNTIME     ON)


include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${Boost_INCLUDE_DIRS} ${MOOS_INCLUDE_DIRS})

set(src
pyMOOS.cpp
) 

add_library(pymoos SHARED ${src})

set_target_properties( pymoos
    PROPERTIES
    SUFFIX ".so"
)

set_target_properties(pymoos
  PROPERTIES
  PREFIX "")
  
  
  
file(GLOB ExampleFiles ${CMAKE_SOURCE_DIR}/examples/*.py)
add_custom_target(copy)
get_target_property(pymoosLocation pymoos LOCATION)
get_filename_component(pymoosDir ${pymoosLocation} PATH)
foreach(ExampleFile ${ExampleFiles})
  add_custom_command(TARGET copy PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E
                         copy ${ExampleFile} ${pymoosDir})
endforeach()
add_dependencies(pymoos copy)  
  



TARGET_LINK_LIBRARIES(pymoos ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} ${MOOS_LIBRARIES})


                       
                       