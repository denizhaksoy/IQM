cmake_minimum_required(VERSION 3.20)
project(IQMFFT256Inverse LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Vulkan REQUIRED)
get_filename_component(IQM_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
add_executable(fft256-inverse-test ../verify_fft256_inverse.cpp
    "${IQM_ROOT}/bin/shared/vulkan_res.cpp"
    "${IQM_ROOT}/bin/IQM/vulkan_instance.cpp"
    "${IQM_ROOT}/src/base/vulkan_runtime.cpp")
target_compile_definitions(fft256-inverse-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(fft256-inverse-test PRIVATE "${IQM_ROOT}/include"
    "${IQM_ROOT}/bin/shared" "${IQM_ROOT}/bin/IQM")
target_link_libraries(fft256-inverse-test PRIVATE Vulkan::Vulkan)
if(MSVC)
    target_compile_options(fft256-inverse-test PRIVATE /EHsc /fp:strict)
else()
    target_compile_options(fft256-inverse-test PRIVATE -ffp-contract=off)
endif()
add_executable(fft256-filter-test ../verify_fft256_filter.cpp
    "${IQM_ROOT}/src/hdrvdp3/mtf.cpp"
    "${IQM_ROOT}/bin/shared/vulkan_res.cpp"
    "${IQM_ROOT}/bin/IQM/vulkan_instance.cpp"
    "${IQM_ROOT}/src/base/vulkan_runtime.cpp")
target_compile_definitions(fft256-filter-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(fft256-filter-test PRIVATE "${IQM_ROOT}/include"
    "${IQM_ROOT}/bin/shared" "${IQM_ROOT}/bin/IQM")
target_link_libraries(fft256-filter-test PRIVATE Vulkan::Vulkan)
if(MSVC)
    target_compile_options(fft256-filter-test PRIVATE /EHsc /fp:strict)
else()
    target_compile_options(fft256-filter-test PRIVATE -ffp-contract=off)
endif()
