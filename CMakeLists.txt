cmake_minimum_required(VERSION 3.0.0)
project(socket-communication VERSION 0.1.0)

include(CTest)
enable_testing()

#Include
include_directories(${PROJECT_SOURCE_DIR}/others/include)
include_directories(${PROJECT_SOURCE_DIR}/mysocket/include)
include_directories(${PROJECT_SOURCE_DIR}/xsocket/include)
find_package(glog REQUIRED)

#Src
file(GLOB_RECURSE sourcefiles 
# "others/**/*.h" "others/**/*.cpp"
"others/include/lib_json/*.h" "others/src/lib_json/*.cpp"
# "mysocket/**/*.h" "mysocket/**/*.cpp"
"xsocket/**/*.h" "xsocket/**/*.cpp"
# "energy/**/*.h" "energy/**/*.cpp" 
# "net/**/*.cpp" "net/**/*.h" 
# "config/**/*.cpp" "config/**/*.h" 
# "com/**/*.h"  "com/**/*.cpp" "com/**/*.cc"
# "armor/**/*.h" "armor/**/*.cpp"
)
# add_library(mysocket mysocket/src/mysocket.cpp)
# add_executable(socket-communication main.cpp ${sourcefiles})
add_executable(xsocket main.cpp ${sourcefiles})
add_executable(mysocket main_.cpp mysocket/include/mysocket.h mysocket/src/mysocket.cpp ${sourcefiles})
add_executable(socket-test test/test.cpp)
# add_executable(test-cilent test/client.cpp)
# add_executable(test-server server.cpp)

# target_link_libraries(socket-communication glog::glog)
target_link_libraries(mysocket glog::glog)
target_link_libraries(xsocket glog::glog)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
