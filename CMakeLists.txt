cmake_minimum_required(VERSION 2.8)
project(plutosdr)

set(CMAKE_C_FLAGS "-std=c99")
if(CMAKE_BUILD_TYPE MATCHES Debug)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
endif()
file(GLOB SOURCES src/*.c)

add_executable(plutosdr ${SOURCES})

install(TARGETS plutosdr DESTINATION /usr/bin/)

find_library(LIBIIO iio)
find_path(LIBIIO_INCLUDE_DIR iio.h)
include_directories(${LIBIIO_INCLUDE_DIR})
target_link_libraries(plutosdr ${LIBIIO})

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_NAME "plutosdr")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libiio0 (>= 0.21-2)")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Andrey Rodionov <dernasherbrezon@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "small cli program for plutosdr")
set(CPACK_DEBIAN_PACKAGE_SECTION "comm")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/dernasherbrezon/plutosdr")
set(CPACK_DEBIAN_DEBUGINFO_PACKAGE ON)
set(CPACK_DEBIAN_FILE_NAME "plutosdr_${CPACK_DEBIAN_PACKAGE_VERSION}_${CUSTOM_ARCHITECTURE}.deb")
include(CPack)
