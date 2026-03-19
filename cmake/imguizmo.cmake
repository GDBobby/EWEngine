

FetchContent_Declare(
    imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imguizmo"
    BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imguizmo"
)
FetchContent_MakeAvailable(imguizmo)