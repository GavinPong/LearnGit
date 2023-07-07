message(STATUS "#####into " ${CMAKE_CURRENT_SOURCE_DIR} "...")

if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.")
endif()

#set(CMAKE_C_COMPILER "/usr/bin/gcc")
#set(CMAKE_CXX_COMPILER "/usr/bin/g++")

cmake_minimum_required(VERSION 3.0)

project(utiltools C CXX)

#检测生成二进制文件的方式
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
  add_definitions(-DDEBUG)
endif()

#根据二进制生成方式，设置编译选项

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -std=c++11")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -std=c++11")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -std=c++11")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -std=c++11")
endif()

message(STATUS "BITNUM:" ${BITNUM})
if(BITNUM EQUAL 64)
    add_definitions(-D__BIT64__)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -m64")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -m64")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -m32")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -m32")     
endif()

message(STATUS "#############CMAKE_BUILD_TYPE:" ${CMAKE_BUILD_TYPE})
message(STATUS "CMAKE_CXX_FLAGS_DEBUG:" ${CMAKE_CXX_FLAGS_DEBUG})
message(STATUS "CMAKE_CXX_FLAGS_RELEASE:" ${CMAKE_CXX_FLAGS_RELEASE})

if(WIN32 AND NOT MINGW)
message(STATUS "编译环境：WIN32")
elseif(WIN32 AND MINGW)
message(STATUS "编译环境：MINGW")
elseif(UNIX AND NOT ANDROID)
message(STATUS "编译环境：UNIX")
set(PTHREAD_LIB ${PTHREAD_LIB} pthread stdc++)
elseif(ANDROID)
message(STATUS "编译环境：ANDROID")
endif()

message(STATUS "c compile is:" ${CMAKE_C_COMPILER})
message(STATUS "cxx compile is:" ${CMAKE_CXX_COMPILER})

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})
message(STATUS "CMAKE_INSTALL_PREFIX:" ${CMAKE_INSTALL_PREFIX})

option(UTIL_TOOLS_BUILD_TESTS "build util tools tests" ON)
set(BUILD_SHARED_LIBS ON)   # Status of the library is "ON:shared OFF:Static" ####

add_subdirectory(src)

if(UTIL_TOOLS_BUILD_TESTS)
    add_subdirectory(test)
else()
    message(STATUS "UTIL_TOOLS_BUILD_TESTS is OFF")
endif()

message(STATUS "#####exit " ${CMAKE_CURRENT_SOURCE_DIR} "...")