cmake_minimum_required(VERSION 3.20)

project(OceanShader)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
add_executable(OceanShader
    src/main.cpp
 "src/stb.cpp" "src/stb.h"   "src/renderutils.cpp" "src/renderutils.h" "src/camera.cpp")

target_link_libraries(OceanShader PRIVATE
    glfw
    glad::glad
    glm::glm
)