

FetchContent_Declare(
    imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imguizmo"
    BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imguizmo"
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(imguizmo)