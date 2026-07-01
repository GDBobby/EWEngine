
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
  	message(WARNING "!!!!FAILED!!!! to find ewe_framework_path in .env.cmake, using FetchContent")
	FetchContent_Declare(
		EWFramework
		GIT_REPOSITORY https://github.com/GDBobby/VulkanFramework
		SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/ewf"
        GIT_PROGRESS   TRUE
		GIT_TAG main

		GIT_SUBMODULES_RECURSIVE TRUE
	)
	FetchContent_MakeAvailable(EWFramework)

	set(EWFramework_LIB EWFramework)
endif()