
find_package(Python REQUIRED)

set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/common/shaders/glsl")

if(CUSTOM_GLSLC_PATH AND EXISTS "${CUSTOM_GLSLC_PATH}")
    set(GLSLC_EXECUTABLE "${CUSTOM_GLSLC_PATH}")
    message(STATUS "Using glslc from env.cmake: ${GLSLC_EXECUTABLE}")
else()
    find_program(GLSLC_SYSTEM_EXECUTABLE glslc)
    
    if(GLSLC_SYSTEM_EXECUTABLE)
        set(GLSLC_EXECUTABLE "${GLSLC_SYSTEM_EXECUTABLE}")
        message(STATUS "Found glslc in system PATH: ${GLSLC_EXECUTABLE}")
    else()
        message(WARNING "glslc was NOT found in system PATH or env.cmake! Shader compilation will fail if attempted.")
        set(GLSLC_EXECUTABLE "glslc")
    endif()
endif()

add_custom_target(compile_shaders ALL
    COMMAND ${Python_EXECUTABLE} 
            "${CMAKE_CURRENT_SOURCE_DIR}/common/compile_shaders.py" 
            "${SHADER_SRC_DIR}"
			"${GLSLC_EXECUTABLE}"
    COMMENT "Checking and compiling outdated GLSL shaders..."
    VERBATIM
)
add_dependencies(${PROJECT_NAME} compile_shaders)