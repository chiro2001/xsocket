cmake_minimum_required(VERSION 3.0.0)
project(socket-communication VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(socket-communication main.cpp)
add_executable(socket-test test.cpp)
# add_compile_options(-lws2_32)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
