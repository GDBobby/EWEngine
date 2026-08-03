
if(DEFINED GLTF_PATH AND EXISTS "${GLTF_PATH}/CMakeLists.txt")
    message(STATUS "~~~~FOUND~~~~ Using local LAB from ${GLTF_PATH}")
    add_subdirectory(${GLTF_PATH} ${GLTF_PATH})
    set(gltf2cpp_LIB gltf2cpp)
else()
    Message(STATUS "!!!!FAILED!!!! to find gltf_path in .env.cmake, using FetchContent")
    FetchContent_Declare(
        gltf2cpp
        GIT_REPOSITORY https://github.com/GDBobby/gltf2cpp
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/gltf2cpp"
        GIT_TAG main
        GIT_PROGRESS   TRUE
    )
    FetchContent_MakeAvailable(gltf2cpp)
endif()