cmake_minimum_required(VERSION 2.8)
project(frog)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -O3 -Wall")
set(SRC_PATH ${PROJECT_SOURCE_DIR}/src)
include_directories(${SRC_PATH})
message(STATUS "${SRC_PATH}")

find_package(Threads REQUIRED)
find_package(Boost 1.50.0 COMPONENTS system thread coroutine context filesystem
  date_time REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

enable_testing()
add_subdirectory(tests)
