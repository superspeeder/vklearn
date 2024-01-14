cmake_minimum_required(VERSION 3.27)
project(vklearn)

set(CMAKE_CXX_STANDARD 23)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(glm CONFIG REQUIRED)


find_package(spdlog CONFIG REQUIRED)

add_executable(vklearn src/main.cpp
        src/main.hpp)
target_include_directories(vklearn PRIVATE src/ ${Stb_INCLUDE_DIR})
target_link_libraries(vklearn Vulkan::Vulkan glfw glm::glm spdlog::spdlog)