cmake_minimum_required(VERSION 3.29)
project(IQM-LibBase)

add_library(IQM-LibBase STATIC vulkan_runtime.cpp colorize.cpp)
add_library(IQM::LibBase ALIAS IQM-LibBase)

find_package(Vulkan REQUIRED)

target_include_directories(IQM-LibBase
        PUBLIC "../../include"
        PRIVATE "../../shaders_out"
)

# target_compile_options(IQM-LibBase PRIVATE "-Wall;-Wextra")
if(MSVC)
    target_compile_options(IQM-LibBase PRIVATE /W4)
else()
    target_compile_options(IQM-LibBase PRIVATE -Wall -Wextra)
endif()
target_link_libraries(IQM-LibBase PUBLIC Vulkan::Vulkan)