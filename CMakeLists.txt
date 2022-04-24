cmake_minimum_required(VERSION 3.1.0)
project(mycp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

include_directories(include)
set(LIBS stdc++fs
    glog
    gflags)

add_library(mycp src/mycp.cpp)

add_executable(main src/main.cpp)
target_link_libraries(main mycp ${LIBS})