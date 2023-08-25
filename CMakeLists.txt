cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(UNIX AND NOT APPLE AND NOT ANDROID)
    # Non-standard preprocessor definition added by gcc:
	add_definitions(-Ulinux)
	set(LINUX TRUE)
endif()

# Macro for stripping library from debug symbols and moving them to a separate file
macro(dump_debug_symbols_and_strip __target)
if(LINUX OR ANDROID)
	add_custom_command(TARGET ${__target} POST_BUILD
		COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:${__target}> $<TARGET_FILE:${__target}>.debug
		COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded $<TARGET_FILE:${__target}>
		COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE:${__target}>.debug $<TARGET_FILE:${__target}>
		COMMENT "Striping ${__target} and generating debug symbols"
	)
elseif(MACOS)
	if(NOT ${CMAKE_GENERATOR} STREQUAL "Xcode")
		# XCode will generate dsyms by its configuration; ninja generator must strip manually:
		add_custom_command(TARGET ${__target} POST_BUILD
			COMMAND dsymutil $<TARGET_FILE:${__target}> -o $<TARGET_FILE:${__target}>.dSYM
			COMMAND ${CMAKE_STRIP} -S $<TARGET_FILE:${__target}>
			COMMENT "Striping ${__target} and generating debug symbols"
		)
	endif()
endif()
endmacro()

function(install_debuginfo __target __destination)
	if(WINDOWS)
		install(
			FILES $<TARGET_PDB_FILE:${__target}>
			DESTINATION ${__destination}
		)
	else(WINDOWS)
		if(LINUX OR ANDROID)
			set(DEBUG_SYMBOLS_EXTENSION "debug")
		elseif(MACOS)
			set(DEBUG_SYMBOLS_EXTENSION "dSYM")
		endif()
		install(
			FILES $<TARGET_FILE:${__target}>.${DEBUG_SYMBOLS_EXTENSION}
			DESTINATION ${__destination}
		)
	endif(WINDOWS)
endfunction()

if(LINUX)
	set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif(LINUX)


set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

project(dolbyio-realtime-media-extensions
	VERSION 1.0.0
)

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

set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES ".git;CMakeLists.txt.user;ext-lib;setup/linux.sh;setup/internal;install_dir;build_dir;build_docker_dir")
include(CPack)
