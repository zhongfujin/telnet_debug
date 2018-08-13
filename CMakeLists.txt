cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
set(CMAKE_BUILD_TYPE "Debug")
set(PROJECT_NAME telnet_ser)
if(CMAKE_COMPILER_IS_GNUCC)
	if(ARCH)
		if(ARCH STREQUAL "x86_64")
			message(STATUS "Compiler for ${ARCH} 64-bit")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
		else (ARCH STREQUAL "x86")
			message(STATUS "Compiler for ${ARCH} 32-bit")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
		endif(ARCH STREQUAL "x86_64")
	endif(ARCH)
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -O0 -fPIC -fno-inline")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O0 -fPIC -fno-inline")
	else(CMAKE_BUILD_TYPE STREQUAL "Release")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O2 -fPIC -fno-inline")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2 -fPIC -fno-inline")
	endif(CMAKE_BUILD_TYPE STREQUAL "Debug")
endif(CMAKE_COMPILER_IS_GNUCC)

include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckTypeSize)
include(CheckFunctionExists)

if(WIN32 OR APPLE)
	message(FATAL_ERROR "Windows or Macos is not supported!")
endif(WIN32 OR APPLE)

check_include_file("features.h" TEL_HAS_FEATURES_H)
check_include_file("errno.h" TEL_HAS_ERRNO_H)
check_include_file("stdio.h" TEL_HAS_STDIO_H)
check_include_file("stdlib.h" TEL_HAS_STDLIB_H)
check_include_file("string.h" TEL_HAS_STRING_H)
check_function_exists("strnlen" TEL_HAS_STRNLEN_FN)
check_function_exists("strtok_r" TEL_HAS_STRTOKR_FN)
check_include_file("stddef.h" TEL_HAS_STDDEF_H)
check_include_file("stdint.h" TEL_HAS_STDINT_H)
check_include_file("stdarg.h" TEL_HAS_STDARG_H)
check_include_file("stdbool.h" TEL_HAS_STDBOOL_H)
check_include_file("time.h" TEL_HAS_TIME_H)
check_include_file("sys/time.h" TEL_HAS_SYS_TIME_H)
check_include_file("sys/types.h" TEL_HAS_SYS_TYPES_H)
check_include_file("unistd.h" TEL_HAS_UNISTD_H)
check_include_file("sys/socket.h" TEL_HAS_SYS_SOCKET_H)
check_include_file("arpa/telnet.h" TEL_HAS_ARPR_TELNET_H)
check_include_file("elf.h" TEL_HAS_ELF_H)
check_include_file("assert.h" TEL_HAS_ASSERT_H)
check_include_file("limits.h" TEL_HAS_LIMITS_H)
check_include_file("pthread.h" TEL_HAS_PTHREAD_H)

MACRO(ADD_PRECOMPILED_HEADER telnet_ser "${CMAKE_CURRENT_SOURCE_DIR}/inc")
ADD_GCC_PRECOMPILED_HEADER(telnet_ser "${CMAKE_CURRENT_SOURCE_DIR}/inc")
ENDMACRO(ADD_PRECOMPILED_HEADER)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)



set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${CMAKE_CURRENT_BINARY_DIR}/bin")

add_subdirectory("src")
add_subdirectory("test")





		


