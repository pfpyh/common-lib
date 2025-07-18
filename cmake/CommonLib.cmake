function(compile_flatbuffer_schemas TARGET_NAME SCHEMA_DIR OUTPUT_DIR)
    # 디버그 출력
    message(STATUS "Compiling FlatBuffer schemas for target: ${TARGET_NAME}")
    message(STATUS "Schema directory: ${SCHEMA_DIR}")
    message(STATUS "Output directory: ${OUTPUT_DIR}")
    
    # 스키마 파일 찾기
    file(GLOB_RECURSE SCHEMA_FILES "${SCHEMA_DIR}/*.fbs")
    message(STATUS "Found schema files: ${SCHEMA_FILES}")
    
    set(GENERATED_HEADERS "")
    foreach(schema_file ${SCHEMA_FILES})
        get_filename_component(schema_name ${schema_file} NAME_WE)
        set(generated_header "${OUTPUT_DIR}/${schema_name}_generated.h")
        
        add_custom_command(
            OUTPUT ${generated_header}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
            COMMAND $<TARGET_FILE:flatc> --cpp -o ${OUTPUT_DIR} ${schema_file}
            DEPENDS flatc ${schema_file}
            COMMENT "Generating FlatBuffer header for ${schema_file}"
        )
        
        list(APPEND GENERATED_HEADERS ${generated_header})
    endforeach()
    
    message(STATUS "Generated headers will be: ${GENERATED_HEADERS}")
    
    # 커스텀 타겟 생성
    add_custom_target(${TARGET_NAME}_schemas DEPENDS ${GENERATED_HEADERS})
    
    # 라이브러리에 의존성 추가
    if(TARGET ${TARGET_NAME})
        add_dependencies(${TARGET_NAME} ${TARGET_NAME}_schemas)
        target_include_directories(${TARGET_NAME} PUBLIC ${OUTPUT_DIR})
    endif()
endfunction()

macro(add_flatbuffer_schemas TARGET_NAME)
    set(options "")
    set(oneValueArgs SCHEMA_DIR OUTPUT_DIR)
    set(multiValueArgs SCHEMA_FILES)
    cmake_parse_arguments(FB_SCHEMA "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    if(NOT FB_SCHEMA_OUTPUT_DIR)
        set(FB_SCHEMA_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    endif()
    
    if(FB_SCHEMA_SCHEMA_DIR)
        compile_flatbuffer_schemas(${TARGET_NAME} ${FB_SCHEMA_SCHEMA_DIR} ${FB_SCHEMA_OUTPUT_DIR})
    endif()
endmacro()