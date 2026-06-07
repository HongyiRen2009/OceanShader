cmake_minimum_required(VERSION 3.20)

project(OceanShader)

set(CMAKE_CXX_STANDARD 17)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
add_executable(OceanShader
    src/main.cpp
 "src/stb.cpp" "src/stb.h"   "src/renderutils.cpp" "src/renderutils.h" "src/camera.cpp" "src/camera.h"     "src/spectrumManager.cpp" "src/spectrumManager.h")

target_link_libraries(OceanShader PRIVATE
    glfw
    glad::glad
    glm::glm
)
# Automatically copy assets into the final build directory
add_custom_command(TARGET OceanShader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders"
        "$<TARGET_FILE_DIR:OceanShader>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/src/textures"
        "$<TARGET_FILE_DIR:OceanShader>/textures"
    COMMENT "Copying shaders and textures to build directory..."
)