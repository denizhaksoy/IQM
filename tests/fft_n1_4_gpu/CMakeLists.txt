cmake_minimum_required(VERSION 3.29)
project(HDRVDP3N14 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
get_filename_component(IQM_ROOT "../.." ABSOLUTE)
find_package(Vulkan REQUIRED)
add_executable(n1-4-probe
 ../verify_fft_n1_4_gpu.cpp
 ../fft_n1_4_gpu.cpp
 ${IQM_ROOT}/src/base/vulkan_runtime.cpp
 ${IQM_ROOT}/bin/shared/vulkan_res.cpp
 ${IQM_ROOT}/bin/IQM/vulkan_instance.cpp)
target_include_directories(n1-4-probe PRIVATE ${IQM_ROOT}/include ${IQM_ROOT}/bin/shared ${IQM_ROOT}/bin/IQM)
target_compile_definitions(n1-4-probe PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_link_libraries(n1-4-probe PRIVATE Vulkan::Vulkan)
if(MSVC)
 target_compile_options(n1-4-probe PRIVATE /fp:strict /EHsc /W4)
else()
 target_compile_options(n1-4-probe PRIVATE -ffp-contract=off)
endif()
