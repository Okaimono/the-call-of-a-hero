cmake_minimum_required(VERSION 3.18)
project(coah-engine LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.cu)

add_executable(coah-engine ${SOURCES})

set_target_properties(coah-engine PROPERTIES
    CUDA_ARCHITECTURES 89
    CUDA_SEPARABLE_COMPILATION ON
)

target_include_directories(coah-engine PRIVATE
    src
    include
    ${Vulkan_INCLUDE_DIRS}
    /usr/include/glm
)

target_link_libraries(coah-engine
    glfw
    ${Vulkan_LIBRARIES}
    dl
    pthread
)