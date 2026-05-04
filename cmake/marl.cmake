
FetchContent_Declare(
	marl
	GIT_REPOSITORY https://github.com/google/marl.git
	GIT_TAG main
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/marl"
	GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(marl)