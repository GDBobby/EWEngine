
set(XXHASH_BUILD_XXHSUM OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
	xxhash
	GIT_REPOSITORY https://github.com/GDBobby/xxHash-constexpr.git
	GIT_TAG        dev
)
FetchContent_MakeAvailable(xxhash)