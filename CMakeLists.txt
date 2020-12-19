cmake_minimum_required(VERSION 3.0.0)
project(socket-communication VERSION 0.1.0)

include(CTest)
enable_testing()

#Include
include_directories(${PROJECT_SOURCE_DIR}/others/include)
include_directories(${PROJECT_SOURCE_DIR}/mySocket/include)
find_package(glog REQUIRED)

#Src
file(GLOB_RECURSE sourcefiles 
"others/**/*.h" "others/**/*.cpp"
# "energy/**/*.h" "energy/**/*.cpp" 
# "net/**/*.cpp" "net/**/*.h" 
# "config/**/*.cpp" "config/**/*.h" 
# "com/**/*.h"  "com/**/*.cpp" "com/**/*.cc"
# "armor/**/*.h" "armor/**/*.cpp"
)
add_executable(socket-communication main.cpp)
# add_executable(socket-test test.cpp)
add_executable(test-cilent client.cpp)
add_executable(test-server server.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
