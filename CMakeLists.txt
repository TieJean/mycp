cmake_minimum_required(VERSION 3.1.0)
project(mycp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

find_library(aio_LIBRARIES aio)
find_package(Boost COMPONENTS filesystem REQUIRED)

include_directories(include ${Boost_INCLUDE_DIR})
set(LIBS stdc++fs
    glog
    gflags
    aio
    ${Boost_LIBRARIES})

add_library(mycp src/mycp.cpp src/eval.cpp)

add_executable(main src/main.cpp)
target_link_libraries(main mycp ${LIBS})

add_executable(eval_main src/eval_main.cpp)
target_link_libraries(eval_main mycp ${LIBS})

add_executable(flush_main src/flush_main.cpp)
target_link_libraries(flush_main mycp ${LIBS})