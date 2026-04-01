include(./.env.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)
message(STATUS "Local .env.cmake: ${LOCAL_ENV}")

cmake_minimum_required(VERSION 3.25)

project(EWEngine VERSION 2.0.0)

message(STATUS "Project source dir : ${PROJECT_SOURCE_DIR}")
message(STATUS "CMAKE source dir : ${CMAKE_SOURCE_DIR}")
message(STATUS "CMake current list dir : ${CMAKE_CURRENT_LIST_DIR}")
message(STATUS "~~~ CURRENT SOURCE DIR : ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "project name : ${PROJECT_NAME}")
message(STATUS "build location : ${CMAKE_CURRENT_BINARY_DIR}")

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(is_project_root OFF)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	set(is_project_root ON)
else()
	set(is_project_root OFF)
endif()
message(STATUS "is EWEngine project root? ${is_project_root}")

option(EWE_EXAMPLES "Build examples" ${is_project_root})
option(EWE_PROJECTS "Build projects" ${is_project_root})

Message(STATUS "Cmake Generator : ${CMAKE_GENERATOR}")

option(USING_ASAN "address sanitizer" OFF)
if(USING_ASAN)
	message(STATUS "using asan ${PROJECT_NAME}")
	if(MSVC)
		add_compile_options(/fsanitize=address)
    	add_link_options(/fsanitize=address)

        # REQUIRED incompatibility fixes
        add_compile_options(/Zi)

        add_link_options(/INCREMENTAL:NO)

        # Required CRT for ASan
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
	else()
		add_compile_options(-fsanitize=address)
    	add_link_options(-fsanitize=address)
    endif()
endif()

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.c)

file(GLOB_RECURSE HEADER_FILES ${PROJECT_SOURCE_DIR}/include/*.h ${PROJECT_SOURCE_DIR}/include/*.hpp)


include(FetchContent)
if(POLICY CMP0077)
	cmake_policy(SET CMP0077 NEW)
endif()

include(cmake/framework.cmake)
include(cmake/harfbuzz.cmake)
include(cmake/imguizmo.cmake)
include(cmake/lab.cmake)
include(cmake/marl.cmake)
include(cmake/rapidjson.cmake)
include(cmake/tinyobj.cmake)
include(cmake/xxhash.cmake)
include(cmake/imageprocessor.cmake)

option(EWE_IMGUI "imgui" ON)

if(EWE_IMGUI)
	include(cmake/imgui.cmake)
	set(IMGUI_SOURCES
		${imgui_SOURCE_DIR}/imgui.cpp
		${imgui_SOURCE_DIR}/imgui_draw.cpp
		${imgui_SOURCE_DIR}/imgui_tables.cpp
		${imgui_SOURCE_DIR}/imgui_widgets.cpp
		${imgui_SOURCE_DIR}/imgui_demo.cpp
		${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
		${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
	)
else()
	set(IMGUI_SOURCES)
endif()


message(STATUS "found EWFramework [${EWFramework_SOURCE_DIR}]")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADER_FILES} ${IMGUI_SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE
	$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall 
        -Wextra 
        -Wshadow -Werror=shadow
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 
        /we4456 /we4457 /we4458 /we4459
    >
	-freflection
)

if(EWE_IMGUI)
    target_compile_definitions(${PROJECT_NAME} PUBLIC EWE_IMGUI)
endif()


message(STATUS "including directories : ${LinearAlgebra_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${Vulkan_INCLUDE_DIR}
	${rapidjson_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/external/include
	${LinearAlgebra_SOURCE_DIR}
    ${EWFramework_SOURCE_DIR}/include
	${imgui_SOURCE_DIR}
	${ImageProcessor_SOURCE_DIR}/include
)

target_compile_definitions(${PROJECT_NAME} PUBLIC IMGUI_DEFINE_MATH_OPERATORS)

target_precompile_headers(${PROJECT_NAME} PUBLIC 
    <imgui.h>
	<imgui_internal.h>
	"include/EWEngine/Global.h"
)

message(STATUS "linking directories")

target_link_directories(${PROJECT_NAME} PUBLIC
	${Vulkan_LIB_DIR}
	$<$<CONFIG:Debug>:${EWFramework_SOURCE_DIR}/Debug>
	$<$<CONFIG:Release>:${EWFramework_SOURCE_DIR}/Release>
)
message(STATUS "linking libs - ${EWFramework_LIB}")

target_link_libraries(${PROJECT_NAME} PUBLIC
	glfw
	${Vulkan_LIBRARY}
	${GLSLANG_LIBS}
	$<$<CONFIG:Debug>:${AFTERMATH_LIBS}>
	EWFramework
	marl
	harfbuzz
	freetype
	xxhash
	ImageProcessor
)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14)
        target_link_libraries(${PROJECT_NAME} PRIVATE stdc++exp) #gcc requires this?
        #target_link_libraries(${PROJECT_NAME}PRIVATE backtrace)
    endif()
endif()

if(EWE_EXAMPLES)
	message(STATUS "building examples\n\n")
	add_subdirectory(examples)
else()
	message(STATUS "skipping example build\n\n")
endif()

if(EWE_PROJECTS)
	message(STATUS "building projects\n\n")

	if(!UNIX)
		#currently bugged, linux users will need to manually change this line for now.
		#ill address this eventually
		#add_subdirectory(Projects)
	endif()
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADER_FILES})