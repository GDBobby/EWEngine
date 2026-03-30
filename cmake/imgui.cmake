if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/")
    set(imgui_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui" CACHE PATH "")
else()
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/GDBobby/imgui
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui"
        BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui"
        GIT_TAG multiple-simultaneous-glfw-contexts
    )
    FetchContent_MakeAvailable(imgui)
endif()