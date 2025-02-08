cmake_minimum_required(VERSION 3.22)
project(VulkanCopyTesting)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(Vulkan REQUIRED)
find_package(VulkanUtilityLibraries REQUIRED)

add_compile_options(
    -Werror
    -Wall
    -Wextra)

include(FetchContent)
FetchContent_Declare(
    fetch_vk_bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
    GIT_TAG        main
)
FetchContent_MakeAvailable(fetch_vk_bootstrap)

add_subdirectory(src)