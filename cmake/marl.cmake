set(MARL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(MARL_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MARL_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
	marl
	GIT_REPOSITORY https://github.com/google/marl.git
	GIT_TAG main
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/marl"
	GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(marl)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_include_directories(marl BEFORE PRIVATE ${CMAKE_SOURCE_DIR}/win32)
endif()

message(STATUS "source dir : ${CMAKE_SOURCE_DIR}")