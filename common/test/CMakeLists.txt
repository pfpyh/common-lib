set(TARGET_NAME test-${TARGET_NAME})
project(${TARGET_NAME})

include(../../cmake/CommonLib.cmake)
compile_flatbuffer_schemas(${TARGET_NAME}
                          "${CMAKE_CURRENT_SOURCE_DIR}/schema"
                          "${CMAKE_CURRENT_BINARY_DIR}/generated")

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)

set(INCLUDES ${CMAKE_CURRENT_LIST_DIR}/include
             ${GTEST_INCLUDES}
             ${INCLUDES})

set(LINKS ${GTEST_LINKS}
          ${LINKS})

set(DEPENDENCIES ${DEPENDENCIES}
                 common-lib
                 gtest
                 gmock)

add_executable(${TARGET_NAME} ${SOURCES})

target_include_directories(${TARGET_NAME}
                           PRIVATE 
                           ${INCLUDES})

target_link_directories(${TARGET_NAME}
                        PRIVATE 
                        ${LINKS})

target_link_libraries(${TARGET_NAME}
                      PRIVATE 
                      ${DEPENDENCIES})

if(NOT CMAKE_CROSSCOMPILING)
    include(GoogleTest)
    gtest_discover_tests(${TARGET_NAME}) 
else()
    message(STATUS "Cross-compiling detected. Skipping CTest registration for ${TARGET_NAME}.")
endif()

if(ENABLE_COVERAGE)
    include(../../cmake/CodeCoverage.cmake)
    target_enable_coverage(${TARGET_NAME})
    target_enable_coverage(common-lib)   # 라이브러리도 계측 대상에 포함
endif()

