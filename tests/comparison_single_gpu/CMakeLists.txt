cmake_minimum_required(VERSION 3.29)
project(HDRVDP3ComparisonProbe LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
get_filename_component(IQM_ROOT "../.." ABSOLUTE)
find_package(Vulkan REQUIRED)
add_executable(comparison-probe
 ../verify_comparison_single_gpu.cpp
 ${IQM_ROOT}/src/hdrvdp3/comparison_single_gpu.cpp
 ${IQM_ROOT}/src/base/vulkan_runtime.cpp
 ${IQM_ROOT}/bin/shared/vulkan_res.cpp
 ${IQM_ROOT}/bin/IQM/vulkan_instance.cpp)
target_include_directories(comparison-probe PRIVATE ${IQM_ROOT}/include ${IQM_ROOT}/bin/shared ${IQM_ROOT}/bin/IQM)
target_compile_definitions(comparison-probe PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_link_libraries(comparison-probe PRIVATE Vulkan::Vulkan)
add_executable(comparison-quality
 ../verify_comparison_quality.cpp
 ${IQM_ROOT}/src/hdrvdp3/comparison_single_gpu.cpp
 ${IQM_ROOT}/src/hdrvdp3/general_quality_single.cpp
 ${IQM_ROOT}/src/hdrvdp3/mutual_single_gpu.cpp
 ${IQM_ROOT}/src/hdrvdp3/comparison_resize_gpu.cpp
 ${IQM_ROOT}/src/hdrvdp3/quality.cpp
 ${IQM_ROOT}/src/hdrvdp3/csf.cpp
 ${IQM_ROOT}/src/hdrvdp3/resize.cpp
 ${IQM_ROOT}/src/base/vulkan_runtime.cpp
 ${IQM_ROOT}/bin/shared/vulkan_res.cpp
 ${IQM_ROOT}/bin/IQM/vulkan_instance.cpp)
target_include_directories(comparison-quality PRIVATE ${IQM_ROOT}/include ${IQM_ROOT}/bin/shared ${IQM_ROOT}/bin/IQM)
target_compile_definitions(comparison-quality PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_link_libraries(comparison-quality PRIVATE Vulkan::Vulkan)
get_target_property(mutual_sources comparison-quality SOURCES)
list(REMOVE_ITEM mutual_sources ../verify_comparison_quality.cpp)
add_executable(mutual-probe ../verify_mutual_single_gpu.cpp ${mutual_sources})
target_include_directories(mutual-probe PRIVATE ${IQM_ROOT}/include ${IQM_ROOT}/bin/shared ${IQM_ROOT}/bin/IQM)
target_compile_definitions(mutual-probe PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_link_libraries(mutual-probe PRIVATE Vulkan::Vulkan)
if(MSVC)
 target_compile_options(comparison-probe PRIVATE /fp:strict /EHsc /W4)
 target_compile_options(comparison-quality PRIVATE /fp:strict /EHsc /W4)
 target_compile_options(mutual-probe PRIVATE /fp:strict /EHsc /W4)
else()
 target_compile_options(comparison-probe PRIVATE -ffp-contract=off)
 target_compile_options(comparison-quality PRIVATE -ffp-contract=off)
 target_compile_options(mutual-probe PRIVATE -ffp-contract=off)
endif()
