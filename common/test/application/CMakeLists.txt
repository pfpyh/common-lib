set(TARGET_NAME test-application)
project(${TARGET_NAME})

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)

set(INCLUDES ${CMAKE_CURRENT_LIST_DIR}/include
             ${GTEST_INCLUDES}
             ${INCLUDES})

set(DEPENDENCIES ${DEPENDENCIES}
                 common-lib)

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
