cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/inc")
aux_source_directory(src SRC_LIST)

add_executable(telnet_test ${SRC_LIST})
link_directories("${CMAKE_BINARY_CURRENT_DIR}/../lib")
target_link_libraries(telnet_test telnetdbg pthread)






		


