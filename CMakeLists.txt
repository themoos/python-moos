cmake_minimum_required(VERSION 2.6)
project (python-moos)
find_package(MOOS 10 REQUIRED)

FIND_PACKAGE(PythonInterp 2.7)
	if(PYTHONINTERP_FOUND)
     		EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c "import distutils.sysconfig; print distutils.sysconfig.get_config_var('INCLUDEDIR')"  OUTPUT_VARIABLE PYTHON_INCLUDE_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
     		EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c "import distutils.sysconfig; print distutils.sysconfig.get_config_var('LIBDIR')"  OUTPUT_VARIABLE PYTHON_LIB_PATH OUTPUT_STRIP_TRAILING_WHITESPACE) 
		if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")  
     		set(PYTHON_LIBRARIES "${PYTHON_LIB_PATH}/libpython2.7.dylib" CACHE FILEPATH "Path to the libpython2.7.dylib library file" FORCE)
		  	if(IS_DIRECTORY "${PYTHON_INCLUDE_PATH}/Python2.7")
				set(PYTHON_INCLUDE_DIRS "${PYTHON_INCLUDE_PATH}/Python2.7" CACHE FILEPATH "Directory holding the python.h include file" FORCE)
		  	else()
				set(PYTHON_INCLUDE_DIRS "${PYTHON_INCLUDE_PATH}/python2.7" CACHE FILEPATH "Directory holding the python.h include file" FORCE)
		  	endif()
		elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
			set(PYTHON_LIBRARIES "${PYTHON_LIB_PATH}/x86_64-linux-gnu/libpython2.7.so" CACHE FILEPATH "Path to the libpython2.7.so library file" FORCE)
  		  	if(IS_DIRECTORY "${PYTHON_INCLUDE_PATH}/Python2.7")
  				set(PYTHON_INCLUDE_DIRS "${PYTHON_INCLUDE_PATH}/Python2.7" CACHE FILEPATH "Directory holding the python.h include file" FORCE)
  		  	else()
  				set(PYTHON_INCLUDE_DIRS "${PYTHON_INCLUDE_PATH}/python2.7" CACHE FILEPATH "Directory holding the python.h include file" FORCE)
  		 	 endif()
		endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	
       		set(PYTHONLIBS_FOUND TRUE)       
      		message(STATUS "PYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARIES}")
      		message(STATUS "PYTHON_INCLUDE_DIR:FILEPATH=${PYTHON_INCLUDE_DIRS}")
      	
    	else()    
		find_package(PythonLibs) # default to standard cmake approach
    	endif(PYTHONINTERP_FOUND)
  
	if(PYTHONLIBS_FOUND)
		include_directories("${PYTHON_INCLUDE_DIRS}")
	else()
  		message(FATAL_ERROR "Unable to find PythonLibs.")
	endif()

find_package(Boost REQUIRED python)
set(Boost_USE_STATIC_LIBS     ON)
set(Boost_USE_MULTITHREADED    ON)
set(Boost_USE_STATIC_RUNTIME     ON)

if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
endif()

if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
endif()
if(NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
endif()


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


TARGET_LINK_LIBRARIES(pymoos ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} ${MOOS_LIBRARIES})
