cmake_minimum_required(VERSION 3.16)

project(common-lib)

set(CMAKE_CXX_STANDARD 17)

if (WIN32)
    add_definitions(-DWINDOWS)
elseif (UNIX)
    add_definitions(-DLINUX)
endif()

# CommonLib Configuration #################################################
if(NOT DEFINED COMMON_LIB_BUILD_TESTING)
    set(COMMON_LIB_BUILD_TESTING ON)
endif()
message(STATUS "COMMON_LIB_BUILD_TESTING = ${COMMON_LIB_BUILD_TESTING}")

if(NOT DEFINED COMMON_LIB_STRICT_MODE)
    set(COMMON_LIB_STRICT_MODE ON)
endif()
message(STATUS "COMMON_LIB_STRICT_MODE = ${COMMON_LIB_STRICT_MODE}")

set(EVENT_THREADS 4 CACHE STRING "Number of threads used by EventBus (default: 4)")
message(STATUS "COMMON_LIB_EVENT_THREADS = ${EVENT_THREADS}")
###########################################################################

# GoogleTest ##############################################################
set(GTEST_ROOT ${CMAKE_CURRENT_LIST_DIR}/libs/googletest)
set(GTEST_INCLUDES ${GTEST_ROOT}/googletest/include
                   ${GTEST_ROOT}/googlemock/include)
set(GTEST_LINKS ${CMAKE_CURRENT_BINARY_DIR}/lib)
###########################################################################

set(INCLUDES ${CMAKE_CURRENT_LIST_DIR}/include
             ${CMAKE_CURRENT_LIST_DIR}
             ${INCLUDES})

set(LINKS ${CMAKE_CURRENT_BINARY_DIR}
          ${CMAKE_CURRENT_BINARY_DIR}/Debug
          ${CMAKE_CURRENT_BINARY_DIR}/Release
          ${CH341_LIB_DIR}
          ${LINKS})

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)

if(COMMON_LIB_STRICT_MODE)
    add_definitions(-DSTRICT_MODE_ENABLED=1)
else()
    add_definitions(-DSTRICT_MODE_ENABLED=0)
endif()

include(CTest)
enable_testing()

add_subdirectory(common)
add_subdirectory(extension)
add_subdirectory(extern)