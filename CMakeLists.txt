cmake_minimum_required(VERSION 3.16)

set(TARGET_NAME common-lib)
set(CMAKE_CXX_STANDARD 17)

project(${TARGET_NAME})

include(cmake/CommonLib.cmake)
include(CTest)
enable_testing()

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

if (WIN32)

endif()

if (WIN32)
    add_definitions(-DWINDOWS)
    #add_compile_options(/Wall)

    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin/${OUTPUTCONFIG})
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib/${OUTPUTCONFIG})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib/${OUTPUTCONFIG})
    endforeach()

    if(DEFINED PYTHON_BASE_DIR)
        message("PYTHON_BASE_DIR = ${PYTHON_BASE_DIR}")
        set(PYTHON_BUILD ON)
        set(PYBIND_BASE_DIR ${PYTHON_BASE_DIR}/Lib/site-packages/pybind11)
        set(CMAKE_PREFIX_PATH ${PYBIND_BASE_DIR}/share/cmake/pybind11
                              ${CMAKE_PREFIX_PATH})
        
        find_package(pybind11 REQUIRED)
        find_package(Python3 COMPONENTS Interpreter Development)
    endif()

    set(CH341_BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs/CH341)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # 64-bit build
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
            set(CH341_LIB_DIR ${CH341_BASE_PATH}/arm64)
            set(CH341_LIB_NAME CH341DLLA64.LIB)
        else()
            set(CH341_LIB_DIR ${CH341_BASE_PATH}/amd64)
            set(CH341_LIB_NAME CH341DLLA64.LIB)
        endif()
    else()
        # 32-bit build
        set(CH341_LIB_DIR ${CH341_BASE_PATH}/i386)
        set(CH341_LIB_NAME CH341DLL.LIB)
    endif()

    set(CH341_LIBRARY ${CH341_LIB_DIR}/${CH341_LIB_NAME})
elseif (UNIX)
    add_definitions(-DLINUX)
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

set(INCLUDES ${CMAKE_CURRENT_LIST_DIR}/include
             ${CMAKE_CURRENT_LIST_DIR}
             ${INCLUDES})

set(LINKS ${CMAKE_CURRENT_BINARY_DIR}
          ${CMAKE_CURRENT_BINARY_DIR}/Debug
          ${CMAKE_CURRENT_BINARY_DIR}/Release
          ${CH341_LIB_DIR}
          ${LINKS})

set(DEPENDENCIES FlatBuffers
                 ${CH341_LIB_NAME})

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)

add_library(${TARGET_NAME} SHARED ${SOURCES})

target_include_directories(${TARGET_NAME} 
                           PRIVATE ${INCLUDES})

target_link_directories(${TARGET_NAME}
                        PRIVATE ${LINKS})

target_link_libraries(${TARGET_NAME} 
                      PRIVATE ${DEPENDENCIES})

target_compile_definitions(${TARGET_NAME} 
                           PRIVATE 
                           BUILDING_COMMON_LIB
                           PUBLIC 
                           EVENT_THREADS=${EVENT_THREADS})

if(COMMON_LIB_STRICT_MODE)
    target_compile_definitions(${TARGET_NAME} PUBLIC STRICT_MODE_ENABLED 1)
else()
    target_compile_definitions(${TARGET_NAME} PUBLIC STRICT_MODE_ENABLED 0)
endif()

# pybind11 ################################################################
if(PYTHON_BUILD)
    set(PY_TARGET_NAME py_common_lib)

    set(INCLUDES ${PYTHON_BASE_DIR}/include
                 ${PYBIND_BASE_DIR}/include
                 ${INCLUDES})

    set(LINKS ${PYTHON_BASE_DIR}/libs
              ${LINKS})

    set(DEPENDENCIES ${TARGET_NAME}
                     pybind11::module 
                     Python3::Python
                     ${DEPENDENCIES}) 

    file(GLOB_RECURSE PY_SOURCES ${CMAKE_CURRENT_LIST_DIR}/python/*.cpp)

    pybind11_add_module(${PY_TARGET_NAME} SHARED ${PY_SOURCES})

    target_include_directories(${PY_TARGET_NAME} 
                               PRIVATE ${INCLUDES})

    target_link_directories(${PY_TARGET_NAME}
                            PRIVATE ${LINKS})    
                            
    target_link_libraries(${PY_TARGET_NAME} 
                          PRIVATE ${DEPENDENCIES})

    target_compile_definitions(${PY_TARGET_NAME}
                               PRIVATE EVENT_THREADS=${EVENT_THREADS})
endif()
###########################################################################

if(WIN32)
    file(GLOB CH341_DLLS "${CH341_LIB_DIR}/*.dll")
    foreach(DLL_FILE ${CH341_DLLS})
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           ${DLL_FILE}
                           ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/
                           COMMENT "Copying CH341 DLL to bin/$<CONFIG>/")
    endforeach()
endif()

add_subdirectory(libs)
if(COMMON_LIB_BUILD_TESTING)
    add_subdirectory(test)
endif()
