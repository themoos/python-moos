cmake_minimum_required(VERSION 2.8.12)
project (python-moos)
find_package(MOOS 10 REQUIRED)


add_subdirectory(pybind11)
pybind11_add_module(pymoos src/pyMOOS.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${MOOS_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIR})
target_link_libraries(pymoos ${PYTHON_LIBRARIES} ${MOOS_LIBRARIES})
