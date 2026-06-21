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