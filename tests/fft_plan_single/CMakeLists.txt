cmake_minimum_required(VERSION 3.29)
project(HDRVDP3SingleFFTPlan LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
add_executable(fft-plan-probe ../inspect_fft_single_plan.cpp)
target_include_directories(fft-plan-probe PRIVATE ../../include)
if(MSVC)
 target_compile_options(fft-plan-probe PRIVATE /fp:strict /EHsc /W4)
endif()

add_executable(fft-flops-probe ../inspect_fft_single_flops.cpp)
target_include_directories(fft-flops-probe PRIVATE ../../include)
if(MSVC)
 target_compile_options(fft-flops-probe PRIVATE /fp:strict /EHsc /W4)
endif()

add_executable(fft720-cpu-graph ../diagnose_fft720_cpu_graph.cpp)
target_include_directories(fft720-cpu-graph PRIVATE ../../include)
if(MSVC)
 target_compile_options(fft720-cpu-graph PRIVATE /fp:strict /EHsc /W4)
endif()

add_executable(fft-native-twiddles ../inspect_fft_native_twiddles.cpp)
target_include_directories(fft-native-twiddles PRIVATE ../../include)

add_executable(fft720-strided-graph ../diagnose_fft720_strided_graph.cpp)
target_include_directories(fft720-strided-graph PRIVATE ../../include)
if(MSVC)
 target_compile_options(fft720-strided-graph PRIVATE /fp:strict /EHsc /W4)
endif()

add_executable(fft720-plan-twiddles ../inspect_fft720_plan_twiddles.cpp)
target_include_directories(fft720-plan-twiddles PRIVATE ../../include)

add_executable(fft720-contracted-graph ../diagnose_fft720_contracted_graph.cpp)
target_include_directories(fft720-contracted-graph PRIVATE ../../include)
if(MSVC)
 target_compile_options(fft720-contracted-graph PRIVATE /fp:strict /EHsc /W4)
endif()

add_executable(fft-next-plans ../inspect_fft_next_plans.cpp)
target_include_directories(fft-next-plans PRIVATE ../../include)

add_executable(fft24-layouts ../inspect_fft24_layouts.cpp)
target_include_directories(fft24-layouts PRIVATE ../../include)

add_executable(fft7680-layouts ../inspect_fft7680_layouts.cpp)
target_include_directories(fft7680-layouts PRIVATE ../../include)

add_executable(fft4k-subplans ../inspect_fft4k_subplans.cpp)
target_include_directories(fft4k-subplans PRIVATE ../../include)
