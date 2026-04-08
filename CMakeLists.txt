cmake_minimum_required(VERSION 3.16)
project(coah-engine)
set(CMAKE_CXX_STANDARD 17)
find_package(Vulkan REQUIRED)
file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(coah-engine ${SOURCES})
target_include_directories(coah-engine PRIVATE
    src
    ${Vulkan_INCLUDE_DIRS}
    /usr/include/glm
)
target_link_libraries(coah-engine
    glfw
    ${Vulkan_LIBRARIES}
    dl
    pthread
)