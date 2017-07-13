cmake_minimum_required(VERSION 2.8.12)
project (python-moos)

find_package(MOOS 10 REQUIRED)

add_subdirectory(pybind11)
pybind11_add_module(pymoos src/pyMOOS.cpp)
include_directories(${MOOS_INCLUDE_DIRS})
target_link_libraries(pymoos ${MOOS_LIBRARIES})
