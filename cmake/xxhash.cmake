
set(XXHASH_BUILD_XXHSUM OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
	xxhash
	GIT_REPOSITORY https://github.com/GDBobby/xxHash-constexpr.git
	GIT_TAG        dev
)
FetchContent_MakeAvailable(xxhash)


#i dont currently have time to make xxHash constexpr compatible
#if(DEFINED XXHASH_PATH)
	#add_subdirectory(${XXHASH_PATH}/build/cmake "${XXHASH_PATH}/build")
	#set(xxhash_LIB xxhash)
#else()
	#message(STATUS "XXHASH_PATH - ${XXHASH_PATH}")
  	#message(FATAL_ERROR "!!!!FAILED!!!! to find XXHASH_PATH in .env.cmake, using FetchContent. until the framework is a git submodule, this is an error")
	
	#FetchContent_Declare(
	#	xxhash
	#	GIT_REPOSITORY https://github.com/GDBobby/xxHash-constexpr.git
	#	GIT_TAG        dev
	#)

	#FetchContent_MakeAvailable(xxhash)
#endif()