if(DEFINED IMGUI_PATH)
else()
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/GDBobby/imgui
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui"
        BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui"
        GIT_TAG master
        GIT_PROGRESS   TRUE
    )
    FetchContent_MakeAvailable(imgui)
    set(IMGUI_PATH ${imgui_SOURCE_DIR} CACHE PATH "")
endif()