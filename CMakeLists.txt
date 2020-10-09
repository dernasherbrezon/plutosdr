cmake_minimum_required(VERSION 2.8)
project(plutosdrCli)

set(CMAKE_C_FLAGS "-std=c99")
if(CMAKE_BUILD_TYPE MATCHES Debug)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
endif()
file(GLOB SOURCES src/*.c)

add_executable(plutosdrCli ${SOURCES})

install(TARGETS plutosdrCli DESTINATION /usr/bin/)

find_library(LIBIIO iio)
find_path(LIBIIO_INCLUDE_DIR iio.h)
include_directories(${LIBIIO_INCLUDE_DIR})
target_link_libraries(plutosdrCli ${LIBIIO})
