cmake_minimum_required(VERSION 3.16)

set(TARGET_NAME common-lib)
set(CMAKE_CXX_STANDARD 17)

project(${TARGET_NAME})

# GoogleTest ############################################
set(GTEST_ROOT ${CMAKE_CURRENT_LIST_DIR}/libs/googletest)
set(GTEST_INCLUDES ${GTEST_ROOT}/googletest/include
                   ${GTEST_ROOT}/googlemock/include)
set(GTEST_LINKS ${CMAKE_CURRENT_BINARY_DIR}/lib)

message(STATUS GTEST_ROOT=${GTEST_ROOT})
message(STATUS GTEST_INCLUDES=${GTEST_INCLUDES})
message(STATUS GTEST_LINKS=${GTEST_LINKS})
##########################################################

if (WIN32)
    add_definitions(-DWINDOWS)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
    #set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

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
elseif (UNIX)
    add_definitions(-DLINUX)

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

set(INCLUDES ${CMAKE_CURRENT_LIST_DIR}/include
             ${INCLUDES})

set(LINKS ${CMAKE_CURRENT_BINARY_DIR}
          ${CMAKE_CURRENT_BINARY_DIR}/Debug
          ${CMAKE_CURRENT_BINARY_DIR}/Release
          ${LINKS})

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)

add_library(${TARGET_NAME} SHARED ${SOURCES})

target_include_directories(${TARGET_NAME} 
                           PRIVATE ${INCLUDES})

target_link_directories(${TARGET_NAME}
                        PRIVATE ${LINKS})

target_compile_definitions(${TARGET_NAME} 
                           PRIVATE BUILDING_COMMON_LIB)

# pybind11 ################################################
if(PYTHON_BUILD)
    set(PY_TARGET_NAME py_common_lib)

    set(INCLUDES ${PYTHON_BASE_DIR}/include
                 ${PYBIND_BASE_DIR}/include
                 ${INCLUDES})

    set(LINKS ${PYTHON_BASE_DIR}/libs
              ${LINKS})

    set(DEPENDENCIES ${TARGET_NAME}
                     pybind11::module 
                     Python3::Python)

    file(GLOB_RECURSE PY_SOURCES ${CMAKE_CURRENT_LIST_DIR}/python/*.cpp)

    pybind11_add_module(${PY_TARGET_NAME} SHARED ${PY_SOURCES})

    target_include_directories(${PY_TARGET_NAME} 
                               PRIVATE ${INCLUDES})

    target_link_directories(${PY_TARGET_NAME}
                            PRIVATE ${LINKS})

    target_link_libraries(${PY_TARGET_NAME} 
                          PRIVATE ${DEPENDENCIES})
endif()
###########################################################

add_subdirectory(libs)
add_subdirectory(test)
