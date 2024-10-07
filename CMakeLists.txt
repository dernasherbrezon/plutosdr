cmake_minimum_required(VERSION 2.8)
project(plutosdr)

set(CMAKE_C_FLAGS "-std=c99")
if(CMAKE_BUILD_TYPE MATCHES Debug)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

add_executable(pluto_rx src/pluto_rx.c src/pluto_util.c)
add_executable(pluto_tx src/pluto_tx.c src/pluto_util.c)

install(TARGETS pluto_rx DESTINATION /usr/bin/)
install(TARGETS pluto_tx DESTINATION /usr/bin/)

find_library(LIBIIO iio)
find_path(LIBIIO_INCLUDE_DIR iio.h)
include_directories(${LIBIIO_INCLUDE_DIR})
target_link_libraries(pluto_rx ${LIBIIO})
target_link_libraries(pluto_tx ${LIBIIO})
