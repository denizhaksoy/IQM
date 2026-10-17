cmake_minimum_required(VERSION 3.29)
project(IQMFFTParity LANGUAGES CXX)

# Independent validation tools: do not configure or link experimental metrics.
find_package(Vulkan REQUIRED)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
get_filename_component(IQM_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
add_executable(IQM-fft-codelet-test
    ../fft_codelet_gpu_test.cpp
    "${IQM_ROOT}/bin/shared/vulkan_res.cpp"
    "${IQM_ROOT}/bin/IQM/vulkan_instance.cpp"
    "${IQM_ROOT}/src/base/vulkan_runtime.cpp")
target_compile_definitions(IQM-fft-codelet-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(IQM-fft-codelet-test PRIVATE
    "${IQM_ROOT}/include" "${IQM_ROOT}/bin/shared" "${IQM_ROOT}/bin/IQM")
target_include_directories(IQM-fft-codelet-test SYSTEM PRIVATE "${IQM_ROOT}/lib/stb")
target_link_libraries(IQM-fft-codelet-test PRIVATE Vulkan::Vulkan)
if(MSVC)
    target_compile_options(IQM-fft-codelet-test PRIVATE /EHsc)
endif()
if(WIN32)
    add_executable(IQM-fftw-plan-test ../inspect_fftw_plan.cpp)
    if(MSVC)
        target_compile_options(IQM-fftw-plan-test PRIVATE /EHsc)
    endif()
endif()
