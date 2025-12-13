cmake_minimum_required(VERSION 3.22)


project(triangle-hello-world)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_executable(${PROJECT_NAME} "triangle_hello_world.cpp")
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})


target_link_libraries(${PROJECT_NAME} PRIVATE EWEngine)
