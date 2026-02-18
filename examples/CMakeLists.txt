cmake_minimum_required(VERSION 3.22)


project(triangle-hello-world)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME} "triangle_hello_world.cpp")
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})


target_link_libraries(${PROJECT_NAME} PUBLIC EWFramework EWEngine)


set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}"
)