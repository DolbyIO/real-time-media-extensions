cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(UNIX AND NOT APPLE AND NOT ANDROID)
    # Non-standard preprocessor definition added by gcc:
	add_definitions(-Ulinux)
	set(LINUX TRUE)
endif()

if(LINUX)
	set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif(LINUX)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

project(dolbyio-realtime-media-extensions
	VERSION 1.0.0
)

include(cmake/macros.cmake)
include(cmake/awstranscribe.cmake)

find_package(Threads REQUIRED)

# The SDK and the multimedia streaming addon provide the CMake package
# files, which define the imported targets. The recommended way of integrating
# the SDK in a cmake-based project is to use these:
if(NOT DOLBYIO_COMMS_SDK_ROOT_DIR)
	set(DOLBYIO_COMMS_SDK_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/ext-lib/sdk-release-ubuntu-20.04-gcc-gnustl)
endif()

find_package(DolbyioComms REQUIRED
	PATHS ${DOLBYIO_COMMS_SDK_ROOT_DIR}/share/dolbyio/comms/cmake
)

add_subdirectory(src)

find_program(GO_EXEC go REQUIRED
	PATHS /usr/local/go/bin/
)
include(ExternalProject)
ExternalProject_Add(rtme_golang_service
	SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/
	DOWNLOAD_COMMAND ""
	INSTALL_COMMAND ""
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ${GO_EXEC} build -o rtme_service .
	COMMAND mv rtme_service ${CMAKE_CURRENT_BINARY_DIR}
	BUILD_IN_SOURCE True
	BUILD_BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/rtme_service"
	BUILD_ALWAYS True
)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/rtme_service DESTINATION bin)
