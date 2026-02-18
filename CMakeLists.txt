include(./.env.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)
message(STATUS "Local .env.cmake: ${LOCAL_ENV}")

cmake_minimum_required(VERSION 3.24)

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

FetchContent_Declare(
	marl
	GIT_REPOSITORY https://github.com/google/marl.git
	GIT_TAG main
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/marl"
)
FetchContent_MakeAvailable(marl)

FetchContent_Declare(
	rapidjson
	GIT_REPOSITORY https://github.com/Tencent/rapidjson
	GIT_TAG 24b5e7a
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/rapidjson"
)
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "")
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "")
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "")
set(RAPIDJSON_BUILD_CXX11 OFF CACHE BOOL "")
set(RAPIDJSON_BUILD_CXX17 ON CACHE BOOL "")
set(RAPIDJSON_BUILD_THIRDPARTY_GTEST OFF CACHE BOOL "")

FetchContent_MakeAvailable(rapidjson)

FetchContent_Declare(
	freetype
	GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
	GIT_TAG master
)
FetchContent_MakeAvailable(freetype)

set(HB_HAVE_FREETYPE ON CACHE BOOL "" FORCE)
set(HB_BUILD_UTILS OFF CACHE BOOL "" FORCE)
set(HB_BUILD_SUBSET OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  harfbuzz
  GIT_REPOSITORY https://github.com/harfbuzz/harfbuzz.git
  GIT_TAG main
)
FetchContent_MakeAvailable(harfbuzz)

FetchContent_Declare(
	tinyobj
	GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader
	GIT_TAG 2945a96
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/include/tinyobjloader"
)
FetchContent_MakeAvailable(tinyobj)

if(DEFINED EWFRAMEWORK_PATH)
	if(EXISTS "${EWFRAMEWORK_PATH}/CMakeLists.txt")
		message(STATUS "~~~~FOUND~~~~ Using local EWFramework from ${EWFRAMEWORK_PATH}")
		add_subdirectory(${EWFRAMEWORK_PATH} "${EWFRAMEWORK_PATH}/build")
		set(EWFramework_LIB EWFramework)
	else()
		message(ERROR "${EWFRAMEWORK_PATH} does not contain CMakeLists.txt")
	endif()
else()
	message(STATUS "EWFRAMEWORK_PATH - ${EWFRAMEWORK_PATH}")
  	message(FATAL_ERROR "!!!!FAILED!!!! to find ewe_framework_path in .env.cmake, using FetchContent. until the framework is a git submodule, this is an error")
	FetchContent_Declare(
		EWFramework
		GIT_REPOSITORY https://github.com/GDBobby/VulkanFramework
		SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/ewf"
		GIT_TAG main
	)
	FetchContent_MakeAvailable(EWFramework)

	set(EWFramework_LIB EWFramework)
endif()

if(NOT TARGET LinearAlgebra)
	if(DEFINED LAB_PATH AND EXISTS "${LAB_PATH}/CMakeLists.txt")
		message(STATUS "~~~~FOUND~~~~ Using local LAB from ${LAB_PATH}")
		add_subdirectory(${LAB_PATH} ${LAB_PATH})
		set(lab_LIB lab)
	else()
	Message(STATUS "!!!!FAILED!!!! to find lab_path in .env.cmake, using FetchContent")
		FetchContent_Declare(
			lab
			GIT_REPOSITORY https://github.com/GDBobby/LAB
			SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/lab"
			GIT_TAG main
		)
		FetchContent_MakeAvailable(lab)
	endif()
endif()

FetchContent_Declare(
	magicenum
	GIT_REPOSITORY https://github.com/Neargye/magic_enum
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/magicenum"
	GIT_TAG 2e6e8bf
)
FetchContent_MakeAvailable(magicenum)

FetchContent_Declare(
    imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imguizmo"
    BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imguizmo"
)
FetchContent_MakeAvailable(imguizmo)

option(EWE_IMGUI "imgui" ON)

if(EWE_IMGUI)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui"
    BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui"
)
FetchContent_MakeAvailable(imgui)

message(STATUS "imgui location - ${imgui_SOURCE_DIR}")

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

if(EWE_IMGUI)
    target_compile_definitions(${PROJECT_NAME} PUBLIC EWE_IMGUI)
endif()


message(STATUS "including directories")
message(STATUS "magicenum source dir - ${magicenum_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${Vulkan_INCLUDE_DIR}
	${rapidjson_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/external/include
	${LinearAlgebra_SOURCE_DIR}
    ${EWFramework_SOURCE_DIR}/include
	${imgui_SOURCE_DIR}
	${magicenum_SOURCE_DIR}/include
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
)

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