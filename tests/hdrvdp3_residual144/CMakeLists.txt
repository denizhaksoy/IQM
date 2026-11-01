cmake_minimum_required(VERSION 3.29)
project(IQMResidual144Diagnostic LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
find_package(Vulkan REQUIRED)
if(WIN32)
    add_executable(square-native-plan-test ../inspect_fftw_plan.cpp)
    if(MSVC)
        target_compile_options(square-native-plan-test PRIVATE /EHsc /fp:strict)
    endif()
endif()
get_filename_component(IQM_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
function(add_base_diagnostic target source)
    add_executable(${target} "../${source}"
        "${IQM_ROOT}/bin/shared/vulkan_res.cpp"
        "${IQM_ROOT}/bin/IQM/vulkan_instance.cpp"
        "${IQM_ROOT}/src/base/vulkan_runtime.cpp")
    target_compile_definitions(${target} PRIVATE VULKAN_HPP_NO_CONSTRUCTORS ${ARGN})
    target_include_directories(${target} PRIVATE "${IQM_ROOT}/include" "${IQM_ROOT}/bin/shared" "${IQM_ROOT}/bin/IQM")
    target_link_libraries(${target} PRIVATE Vulkan::Vulkan)
    if(MSVC)
        target_compile_options(${target} PRIVATE /EHsc /fp:strict)
    else()
        target_compile_options(${target} PRIVATE -ffp-contract=off)
    endif()
endfunction()
add_base_diagnostic(residual144-test verify_residual120.cpp)
add_base_diagnostic(rgb-projection-gpu-test verify_rgb_projection_gpu.cpp)
add_base_diagnostic(rgb709-projection-gpu-test verify_rgb709_gpu.cpp)
add_base_diagnostic(rgb-comparison-test verify_rgb_comparison.cpp)
target_sources(rgb-comparison-test PRIVATE "${IQM_ROOT}/src/hdrvdp3/pathway_comparison.cpp" "${IQM_ROOT}/src/hdrvdp3/resize.cpp")
add_base_diagnostic(rgb-optical-gpu-test verify_rgb_optical_gpu.cpp)
target_sources(rgb-optical-gpu-test PRIVATE
    "${IQM_ROOT}/src/hdrvdp3/mixed_filter.cpp" "${IQM_ROOT}/src/hdrvdp3/mtf.cpp"
    "${IQM_ROOT}/src/hdrvdp3/spectral.cpp" "${IQM_ROOT}/src/hdrvdp3/spectral_loader.cpp")
target_sources(rgb-optical-gpu-test PRIVATE
    "${IQM_ROOT}/src/hdrvdp3/map_gpu.cpp" "${IQM_ROOT}/src/hdrvdp3/jnd.cpp"
    "${IQM_ROOT}/src/hdrvdp3/csf.cpp" "${IQM_ROOT}/src/hdrvdp3/resize.cpp")
add_base_diagnostic(mean144-test verify_mean120.cpp IQM_MEAN_COUNT=144)
add_base_diagnostic(mean-general-test verify_mean120.cpp IQM_MEAN_DYNAMIC_COUNT)
add_base_diagnostic(base144-pipeline-test verify_residual120_pipeline.cpp IQM_BASE_COUNT=144)
add_base_diagnostic(general-base-pipeline-test verify_residual120_pipeline.cpp IQM_BASE_GENERAL)
add_base_diagnostic(base256-pipeline-test verify_residual120_pipeline.cpp IQM_BASE_GENERAL IQM_BASE256_DIAGNOSTIC)
add_base_diagnostic(general-spyr-test verify_general_spyr.cpp
    IQM_HDRVDP3_SQUARE_SPYR_DIAGNOSTIC IQM_HDRVDP3_GENERAL_SPYR_DIAGNOSTIC IQM_HDRVDP3_GENERAL_MEAN_DIAGNOSTIC)
target_sources(general-spyr-test PRIVATE "${IQM_ROOT}/src/hdrvdp3/pathway_spyr.cpp" "${IQM_ROOT}/src/hdrvdp3/sp0_model.cpp")
add_base_diagnostic(base256-spyr-test verify_general_spyr.cpp IQM_BASE256_DIAGNOSTIC
    IQM_HDRVDP3_SQUARE_SPYR_DIAGNOSTIC IQM_HDRVDP3_SQUARE128_SPYR_DIAGNOSTIC IQM_HDRVDP3_GENERAL_MEAN_DIAGNOSTIC)
target_sources(base256-spyr-test PRIVATE "${IQM_ROOT}/src/hdrvdp3/pathway_spyr.cpp" "${IQM_ROOT}/src/hdrvdp3/sp0_model.cpp")
add_base_diagnostic(square192-fft-test verify_fft256_inverse.cpp)
add_base_diagnostic(square192-normalization-test verify_mixed_normalization.cpp)
add_base_diagnostic(square-filter-chain-test verify_square_filter_chain.cpp IQM_HDRVDP3_SQUARE_FILTER_DIAGNOSTIC IQM_HDRVDP3_SQUARE_SPYR_DIAGNOSTIC)
target_sources(square-filter-chain-test PRIVATE
    "${IQM_ROOT}/src/hdrvdp3/mixed_filter.cpp" "${IQM_ROOT}/src/hdrvdp3/mtf.cpp"
    "${IQM_ROOT}/src/hdrvdp3/map_gpu.cpp" "${IQM_ROOT}/src/hdrvdp3/resize.cpp"
    "${IQM_ROOT}/src/hdrvdp3/csf.cpp" "${IQM_ROOT}/src/hdrvdp3/jnd.cpp"
    "${IQM_ROOT}/src/hdrvdp3/pathway_spyr.cpp" "${IQM_ROOT}/src/hdrvdp3/sp0_model.cpp")
add_base_diagnostic(square80-filter-chain-test verify_square_filter_chain.cpp IQM_SQUARE_SIDE=80
    IQM_HDRVDP3_SQUARE_FILTER_DIAGNOSTIC IQM_HDRVDP3_GENERAL_FILTER_DIAGNOSTIC
    IQM_HDRVDP3_SQUARE_SPYR_DIAGNOSTIC IQM_HDRVDP3_GENERAL_SPYR_DIAGNOSTIC IQM_HDRVDP3_GENERAL_MEAN_DIAGNOSTIC)
target_sources(square80-filter-chain-test PRIVATE
    "${IQM_ROOT}/src/hdrvdp3/mixed_filter.cpp" "${IQM_ROOT}/src/hdrvdp3/mtf.cpp"
    "${IQM_ROOT}/src/hdrvdp3/map_gpu.cpp" "${IQM_ROOT}/src/hdrvdp3/resize.cpp"
    "${IQM_ROOT}/src/hdrvdp3/csf.cpp" "${IQM_ROOT}/src/hdrvdp3/jnd.cpp"
    "${IQM_ROOT}/src/hdrvdp3/pathway_spyr.cpp" "${IQM_ROOT}/src/hdrvdp3/sp0_model.cpp")
add_base_diagnostic(square128-filter-chain-test verify_square_filter_chain.cpp IQM_SQUARE_SIDE=128
    IQM_HDRVDP3_SQUARE_FILTER_DIAGNOSTIC IQM_HDRVDP3_SQUARE128_FILTER_DIAGNOSTIC
    IQM_HDRVDP3_SQUARE_SPYR_DIAGNOSTIC IQM_HDRVDP3_SQUARE128_SPYR_DIAGNOSTIC IQM_HDRVDP3_GENERAL_MEAN_DIAGNOSTIC)
target_sources(square128-filter-chain-test PRIVATE
    "${IQM_ROOT}/src/hdrvdp3/mixed_filter.cpp" "${IQM_ROOT}/src/hdrvdp3/mtf.cpp"
    "${IQM_ROOT}/src/hdrvdp3/map_gpu.cpp" "${IQM_ROOT}/src/hdrvdp3/resize.cpp"
    "${IQM_ROOT}/src/hdrvdp3/csf.cpp" "${IQM_ROOT}/src/hdrvdp3/jnd.cpp"
    "${IQM_ROOT}/src/hdrvdp3/pathway_spyr.cpp" "${IQM_ROOT}/src/hdrvdp3/sp0_model.cpp")
add_base_diagnostic(square-quality-test verify_square_quality.cpp
    IQM_HDRVDP3_SQUARE_FILTER_DIAGNOSTIC IQM_HDRVDP3_SQUARE_SPYR_DIAGNOSTIC
    IQM_HDRVDP3_SQUARE_QUALITY_DIAGNOSTIC)
set(square_quality_modules
    "${IQM_ROOT}/src/hdrvdp3/pathway_quality.cpp" "${IQM_ROOT}/src/hdrvdp3/pathway_pool.cpp"
    "${IQM_ROOT}/src/hdrvdp3/pathway_comparison.cpp" "${IQM_ROOT}/src/hdrvdp3/pathway_masking.cpp"
    "${IQM_ROOT}/src/hdrvdp3/mixed_filter.cpp" "${IQM_ROOT}/src/hdrvdp3/mtf.cpp"
    "${IQM_ROOT}/src/hdrvdp3/map_gpu.cpp" "${IQM_ROOT}/src/hdrvdp3/resize.cpp"
    "${IQM_ROOT}/src/hdrvdp3/csf.cpp" "${IQM_ROOT}/src/hdrvdp3/jnd.cpp"
    "${IQM_ROOT}/src/hdrvdp3/pathway_spyr.cpp" "${IQM_ROOT}/src/hdrvdp3/sp0_model.cpp"
    "${IQM_ROOT}/src/hdrvdp3/quality.cpp")
target_sources(square-quality-test PRIVATE ${square_quality_modules})
target_sources(rgb-optical-gpu-test PRIVATE ${square_quality_modules} "${IQM_ROOT}/src/hdrvdp3/pathway_runtime.cpp")
add_base_diagnostic(rgb-quality-api-test verify_rgb_quality_api.cpp)
target_sources(rgb-quality-api-test PRIVATE ${square_quality_modules}
    "${IQM_ROOT}/src/hdrvdp3/pathway_runtime.cpp" "${IQM_ROOT}/src/hdrvdp3/rgb_quality.cpp"
    "${IQM_ROOT}/src/hdrvdp3/spectral.cpp" "${IQM_ROOT}/src/hdrvdp3/spectral_loader.cpp")
target_sources(rgb709-projection-gpu-test PRIVATE ${square_quality_modules}
    "${IQM_ROOT}/src/hdrvdp3/pathway_runtime.cpp" "${IQM_ROOT}/src/hdrvdp3/rgb_quality.cpp"
    "${IQM_ROOT}/src/hdrvdp3/spectral.cpp" "${IQM_ROOT}/src/hdrvdp3/spectral_loader.cpp")
add_base_diagnostic(rgb709-quality-api-test verify_rgb709_api.cpp)
target_sources(rgb709-quality-api-test PRIVATE ${square_quality_modules}
    "${IQM_ROOT}/src/hdrvdp3/pathway_runtime.cpp" "${IQM_ROOT}/src/hdrvdp3/rgb_quality.cpp"
    "${IQM_ROOT}/src/hdrvdp3/spectral.cpp" "${IQM_ROOT}/src/hdrvdp3/spectral_loader.cpp")
add_executable(rgb-runtime-test ../verify_rgb_runtime.cpp "${IQM_ROOT}/src/hdrvdp3/pathway_runtime.cpp")
add_executable(rgb709-runtime-test ../verify_rgb_runtime.cpp "${IQM_ROOT}/src/hdrvdp3/pathway_runtime.cpp")
foreach(color IN ITEMS BT2020 XYZ)
    add_executable(${color}-runtime-test ../verify_rgb_runtime.cpp "${IQM_ROOT}/src/hdrvdp3/pathway_runtime.cpp")
    target_compile_definitions(${color}-runtime-test PRIVATE IQM_TEST_${color}_RUNTIME)
    target_include_directories(${color}-runtime-test PRIVATE "${IQM_ROOT}/include")
    if(MSVC)
        target_compile_options(${color}-runtime-test PRIVATE /EHsc /fp:strict)
    endif()
endforeach()
target_compile_definitions(rgb709-runtime-test PRIVATE IQM_TEST_RGB709_RUNTIME)
target_include_directories(rgb709-runtime-test PRIVATE "${IQM_ROOT}/include")
if(MSVC)
    target_compile_options(rgb709-runtime-test PRIVATE /EHsc /fp:strict)
endif()
target_include_directories(rgb-runtime-test PRIVATE "${IQM_ROOT}/include")
if(MSVC)
    target_compile_options(rgb-runtime-test PRIVATE /EHsc /fp:strict)
endif()
add_base_diagnostic(general-mean-quality-test verify_square_quality.cpp
    IQM_HDRVDP3_SQUARE_FILTER_DIAGNOSTIC IQM_HDRVDP3_SQUARE_SPYR_DIAGNOSTIC
    IQM_HDRVDP3_SQUARE_QUALITY_DIAGNOSTIC IQM_HDRVDP3_GENERAL_MEAN_DIAGNOSTIC)
target_sources(general-mean-quality-test PRIVATE ${square_quality_modules})
add_base_diagnostic(square80-quality-test verify_square_quality.cpp IQM_SQUARE_SIDE=80
    IQM_HDRVDP3_SQUARE_FILTER_DIAGNOSTIC IQM_HDRVDP3_GENERAL_FILTER_DIAGNOSTIC
    IQM_HDRVDP3_SQUARE_SPYR_DIAGNOSTIC IQM_HDRVDP3_GENERAL_SPYR_DIAGNOSTIC IQM_HDRVDP3_GENERAL_MEAN_DIAGNOSTIC
    IQM_HDRVDP3_SQUARE_QUALITY_DIAGNOSTIC IQM_HDRVDP3_SQUARE80_QUALITY_DIAGNOSTIC)
target_sources(square80-quality-test PRIVATE ${square_quality_modules})
add_base_diagnostic(square128-quality-test verify_square_quality.cpp IQM_SQUARE_SIDE=128
    IQM_HDRVDP3_SQUARE_FILTER_DIAGNOSTIC IQM_HDRVDP3_SQUARE128_FILTER_DIAGNOSTIC
    IQM_HDRVDP3_SQUARE_SPYR_DIAGNOSTIC IQM_HDRVDP3_SQUARE128_SPYR_DIAGNOSTIC IQM_HDRVDP3_GENERAL_MEAN_DIAGNOSTIC
    IQM_HDRVDP3_SQUARE_QUALITY_DIAGNOSTIC IQM_HDRVDP3_SQUARE128_QUALITY_DIAGNOSTIC)
target_sources(square128-quality-test PRIVATE ${square_quality_modules})
option(IQM_BASE144_LEGACY_REGRESSION "Build original120 diagnostic callers" OFF)
if(IQM_BASE144_LEGACY_REGRESSION)
    add_base_diagnostic(quality-production-regression-test verify_pathway_contrast.cpp)
    target_sources(quality-production-regression-test PRIVATE ${square_quality_modules} "${IQM_ROOT}/src/hdrvdp3/pathway_runtime.cpp")
    add_base_diagnostic(spyr-production-regression-test verify_spyr_runtime.cpp)
    target_sources(spyr-production-regression-test PRIVATE "${IQM_ROOT}/src/hdrvdp3/pathway_spyr.cpp")
    add_base_diagnostic(mean120-regression-test verify_mean120.cpp)
    add_base_diagnostic(base120-pipeline-regression-test verify_residual120_pipeline.cpp)
    add_base_diagnostic(mixed-filter-production-regression-test verify_mixed_filter_chain.cpp)
    target_sources(mixed-filter-production-regression-test PRIVATE
        "${IQM_ROOT}/src/hdrvdp3/mixed_filter.cpp" "${IQM_ROOT}/src/hdrvdp3/mtf.cpp")
endif()
