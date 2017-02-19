# Borrowed from morse testing
macro(add_moos_test name)
  set(full_name ${CMAKE_CURRENT_SOURCE_DIR}/${name})
  string(REPLACE "${CMAKE_SOURCE_DIR}/testing/" "" test_name ${full_name})
  add_test(NAME ${test_name}
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py)
endmacro()

add_moos_test(run_close)
add_moos_test(register)
add_moos_test(simple_comms)
