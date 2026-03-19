
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui"
    BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui"
)
FetchContent_MakeAvailable(imgui)