
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