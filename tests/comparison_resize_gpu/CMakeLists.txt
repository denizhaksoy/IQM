cmake_minimum_required(VERSION 3.29)
project(HDRVDP3ComparisonResize LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
get_filename_component(IQM_ROOT "../.." ABSOLUTE)
find_package(Vulkan REQUIRED)
add_executable(resize-probe ../verify_comparison_resize_gpu.cpp
 ${IQM_ROOT}/src/hdrvdp3/comparison_resize_gpu.cpp
 ${IQM_ROOT}/src/hdrvdp3/resize.cpp
 ${IQM_ROOT}/src/base/vulkan_runtime.cpp
 ${IQM_ROOT}/bin/shared/vulkan_res.cpp
 ${IQM_ROOT}/bin/IQM/vulkan_instance.cpp)
target_include_directories(resize-probe PRIVATE ${IQM_ROOT}/include ${IQM_ROOT}/bin/shared ${IQM_ROOT}/bin/IQM)
target_compile_definitions(resize-probe PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_link_libraries(resize-probe PRIVATE Vulkan::Vulkan)
get_target_property(live_sources resize-probe SOURCES)
list(REMOVE_ITEM live_sources ../verify_comparison_resize_gpu.cpp)
add_executable(resize-live-probe ../verify_live_comparison_resize.cpp ${live_sources}
 ${IQM_ROOT}/src/hdrvdp3/pq_single.cpp
 ${IQM_ROOT}/src/hdrvdp3/linear_color_single.cpp
 ${IQM_ROOT}/src/hdrvdp3/spectral.cpp
 ${IQM_ROOT}/src/hdrvdp3/spectral_loader.cpp)
target_include_directories(resize-live-probe PRIVATE ${IQM_ROOT}/include ${IQM_ROOT}/bin/shared ${IQM_ROOT}/bin/IQM ${IQM_ROOT}/lib/stb)
target_compile_definitions(resize-live-probe PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_link_libraries(resize-live-probe PRIVATE Vulkan::Vulkan)
if(MSVC)
 target_compile_options(resize-probe PRIVATE /fp:strict /EHsc /W4)
 target_compile_options(resize-live-probe PRIVATE /fp:strict /EHsc /W4)
else()
 target_compile_options(resize-probe PRIVATE -ffp-contract=off)
 target_compile_options(resize-live-probe PRIVATE -ffp-contract=off)
endif()
