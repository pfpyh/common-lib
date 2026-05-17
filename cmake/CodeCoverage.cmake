option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)

function(target_enable_coverage TARGET_NAME)
    if(NOT ENABLE_COVERAGE)
        return()
    endif()

    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(WARNING "Coverage는 GCC/Clang에서만 지원됩니다.")
        return()
    endif()

    target_compile_options(${TARGET_NAME} PRIVATE --coverage -O0 -g -fprofile-update=atomic)
    target_link_options(${TARGET_NAME} PRIVATE --coverage)
endfunction()

function(add_coverage_target)
    if(NOT ENABLE_COVERAGE)
        return()
    endif()

    cmake_parse_arguments(COV "" "" "DEPENDS" ${ARGN})

    find_program(LCOV_PATH lcov REQUIRED)
    find_program(GENHTML_PATH genhtml REQUIRED)

    set(COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage)
    set(COVERAGE_INFO ${COVERAGE_DIR}/coverage.info)
    set(COVERAGE_FILTERED ${COVERAGE_DIR}/coverage_filtered.info)

    set(EXCLUDE_PATTERNS
        "${CMAKE_SOURCE_DIR}/extern/*"
        "${CMAKE_SOURCE_DIR}/common/test/*"
        "/usr/*"
    )

    set(COVERAGE_EXCLUDE_FILE "${CMAKE_SOURCE_DIR}/cmake/CodeCoverage_exclude.conf")
    if(EXISTS "${COVERAGE_EXCLUDE_FILE}")
        message(STATUS "Coverage exclude patterns loaded from ${COVERAGE_EXCLUDE_FILE}")
        file(STRINGS "${COVERAGE_EXCLUDE_FILE}" EXCLUDE_LINES)
        foreach(line ${EXCLUDE_LINES})
            string(STRIP "${line}" stripped)
            if(stripped STREQUAL "" OR stripped MATCHES "^#")
                continue()
            endif()
            if(stripped MATCHES "^/")
                list(APPEND EXCLUDE_PATTERNS "${stripped}")
            else()
                list(APPEND EXCLUDE_PATTERNS "${CMAKE_SOURCE_DIR}/common/${stripped}")
            endif()
        endforeach()
    endif()

    add_custom_target(coverage
        COMMENT "Generating code coverage report..."

        DEPENDS ${COV_DEPENDS}

        COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_DIR}

        # 빌드 디렉토리 초기화
        COMMAND ${LCOV_PATH}
            --zerocounters
            --directory ${CMAKE_BINARY_DIR}

        # 베이스라인 캡처
        COMMAND ${LCOV_PATH}
            --capture
            --initial
            --directory ${CMAKE_BINARY_DIR}
            --output-file ${COVERAGE_INFO}
            --ignore-errors mismatch,negative,inconsistent
            --branch-coverage 

        # 테스트 실행
        COMMAND ${CMAKE_CTEST_COMMAND}
            --test-dir ${CMAKE_BINARY_DIR}
            --output-on-failure

        # 실행 후 데이터 캡처
        COMMAND ${LCOV_PATH}
            --capture
            --directory ${CMAKE_BINARY_DIR}
            --output-file ${COVERAGE_INFO}
            --ignore-errors mismatch,negative,inconsistent
            --branch-coverage

        # 제외 패턴 적용
        COMMAND ${LCOV_PATH}
            --remove ${COVERAGE_INFO}
            ${EXCLUDE_PATTERNS}
            --output-file ${COVERAGE_FILTERED}
            --ignore-errors unused,inconsistent
            --branch-coverage

        # HTML 리포트 생성
        COMMAND ${GENHTML_PATH}
            ${COVERAGE_FILTERED}
            --output-directory ${COVERAGE_DIR}/html
            --title "common-lib Coverage Report"
            --legend
            --show-details
            --branch-coverage
            --ignore-errors inconsistent,corrupt

        COMMAND ${CMAKE_COMMAND} -E echo
            "Coverage report: ${COVERAGE_DIR}/html/index.html"
    )
endfunction()