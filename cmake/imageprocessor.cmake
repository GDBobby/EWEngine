if(DEFINED IMAGEPROC_PATH)
	if(EXISTS "${IMAGEPROC_PATH}/CMakeLists.txt")
		message(STATUS "~~~~FOUND~~~~ Using local ImageProcessor from ${IMAGEPROC_PATH}")
		add_subdirectory(${IMAGEPROC_PATH} "${IMAGEPROC_PATH}/build")
		set(ImageProcessor_LIB ImageProcessor)
	else()
		message(ERROR "${IMAGEPROC_PATH} does not contain CMakeLists.txt")
	endif()
else()
	message(STATUS "IMAGEPROC_PATH - ${IMAGEPROC_PATH}")
  	message(FATAL_ERROR "!!!!FAILED!!!! to find IMAGEPROC_PATH in .env.cmake, using FetchContent. until the framework is a git submodule, this is an error")
	FetchContent_Declare(
		ImageProcessor
		GIT_REPOSITORY https://github.com/GDBobby/i-spent-a-lot-of-time-coming-up-with-a-good-project-name-so-people-would-know-what-this-project-does
		SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imageprocessor"
		GIT_TAG main
	)
	FetchContent_MakeAvailable(ImageProcessor)

	set(ImageProcessor_LIB ImageProcessor)
endif()