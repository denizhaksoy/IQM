cmake_minimum_required(VERSION 3.29)
project(IQMDimensionPadding LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(frequency-grid-test "${CMAKE_CURRENT_LIST_DIR}/../verify_frequency_grid.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/quality.cpp")
target_include_directories(frequency-grid-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../include")
if(MSVC)
    target_compile_options(frequency-grid-test PRIVATE /EHsc /fp:strict)
else()
    target_compile_options(frequency-grid-test PRIVATE -ffp-contract=off)
endif()
add_executable(spyr-lowpass-test "${CMAKE_CURRENT_LIST_DIR}/../verify_spyr_lowpass.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared/vulkan_res.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM/vulkan_instance.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/base/vulkan_runtime.cpp")
target_compile_definitions(spyr-lowpass-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(spyr-lowpass-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../include" "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared" "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM")
target_link_libraries(spyr-lowpass-test PRIVATE Vulkan::Vulkan)
if(MSVC)
    target_compile_options(spyr-lowpass-test PRIVATE /EHsc /fp:strict)
else()
    target_compile_options(spyr-lowpass-test PRIVATE -ffp-contract=off)
endif()
add_executable(mixed-filter-chain-test "${CMAKE_CURRENT_LIST_DIR}/../verify_mixed_filter_chain.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared/vulkan_res.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM/vulkan_instance.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/base/vulkan_runtime.cpp")
target_compile_definitions(mixed-filter-chain-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(mixed-filter-chain-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../include" "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared" "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM")
target_link_libraries(mixed-filter-chain-test PRIVATE Vulkan::Vulkan)
if(MSVC)
    target_compile_options(mixed-filter-chain-test PRIVATE /EHsc /fp:strict)
else()
    target_compile_options(mixed-filter-chain-test PRIVATE -ffp-contract=off)
endif()
find_package(Vulkan REQUIRED COMPONENTS glslc)
add_executable(jnd-native-order-test "${CMAKE_CURRENT_LIST_DIR}/../verify_jnd_model_words.cpp" "${CMAKE_CURRENT_LIST_DIR}/../jnd_model_native_order.cpp")
target_include_directories(jnd-native-order-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../include")
if(MSVC)
 target_compile_options(jnd-native-order-test PRIVATE /EHsc /fp:strict)
else()
 target_compile_options(jnd-native-order-test PRIVATE -ffp-contract=off)
endif()
add_executable(jnd-production-order-test "${CMAKE_CURRENT_LIST_DIR}/../verify_jnd_model_words.cpp" "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/jnd.cpp")
target_include_directories(jnd-production-order-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../include")
if(MSVC)
 target_compile_options(jnd-production-order-test PRIVATE /EHsc /fp:strict)
else()
 target_compile_options(jnd-production-order-test PRIVATE -ffp-contract=off)
endif()
target_sources(mixed-filter-chain-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/mixed_filter.cpp" "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/mtf.cpp")
add_executable(general-comparison-test "${CMAKE_CURRENT_LIST_DIR}/../verify_general_comparison.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/resize.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared/vulkan_res.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM/vulkan_instance.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/base/vulkan_runtime.cpp")
target_compile_definitions(general-comparison-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(general-comparison-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../include" "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared" "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM")
target_link_libraries(general-comparison-test PRIVATE Vulkan::Vulkan)
if(MSVC)
    target_compile_options(general-comparison-test PRIVATE /EHsc /fp:strict)
else()
    target_compile_options(general-comparison-test PRIVATE -ffp-contract=off)
endif()
add_executable(pathway-jnd-test "${CMAKE_CURRENT_LIST_DIR}/../verify_pathway_jnd.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/jnd.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/map_gpu.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/resize.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/csf.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/quality.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared/vulkan_res.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM/vulkan_instance.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/base/vulkan_runtime.cpp")
target_compile_definitions(pathway-jnd-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(pathway-jnd-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../include" "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared" "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM")
target_link_libraries(pathway-jnd-test PRIVATE Vulkan::Vulkan)
if(MSVC)
    target_compile_options(pathway-jnd-test PRIVATE /EHsc /fp:strict)
else()
    target_compile_options(pathway-jnd-test PRIVATE -ffp-contract=off)
endif()
add_executable(pathway-jnd-native-order-test "${CMAKE_CURRENT_LIST_DIR}/../verify_pathway_jnd.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../jnd_model_native_order.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/map_gpu.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/resize.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/csf.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/quality.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared/vulkan_res.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM/vulkan_instance.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/base/vulkan_runtime.cpp")
target_compile_definitions(pathway-jnd-native-order-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(pathway-jnd-native-order-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../include" "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared" "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM")
target_link_libraries(pathway-jnd-native-order-test PRIVATE Vulkan::Vulkan)
if(MSVC)
    target_compile_options(pathway-jnd-native-order-test PRIVATE /EHsc /fp:strict)
else()
    target_compile_options(pathway-jnd-native-order-test PRIVATE -ffp-contract=off)
endif()
add_executable(pathway-masking-test "${CMAKE_CURRENT_LIST_DIR}/../verify_pathway_masking.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/map_gpu.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/resize.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/csf.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/quality.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared/vulkan_res.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM/vulkan_instance.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/base/vulkan_runtime.cpp")
target_compile_definitions(pathway-masking-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(pathway-masking-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../include" "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared" "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM")
target_link_libraries(pathway-masking-test PRIVATE Vulkan::Vulkan)
if(MSVC)
    target_compile_options(pathway-masking-test PRIVATE /EHsc /fp:strict)
else()
    target_compile_options(pathway-masking-test PRIVATE -ffp-contract=off)
endif()
add_executable(pathway-pool-test "${CMAKE_CURRENT_LIST_DIR}/../verify_pathway_pool.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared/vulkan_res.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM/vulkan_instance.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/base/vulkan_runtime.cpp")
target_compile_definitions(pathway-pool-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(pathway-pool-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../include" "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared" "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM")
target_link_libraries(pathway-pool-test PRIVATE Vulkan::Vulkan)
if(MSVC)
    target_compile_options(pathway-pool-test PRIVATE /EHsc /fp:strict)
else()
    target_compile_options(pathway-pool-test PRIVATE -ffp-contract=off)
endif()
get_filename_component(IQM_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/dimension_padding.spv"
    COMMAND Vulkan::glslc "${IQM_ROOT}/tests/dimension_padding.comp" --target-env=vulkan1.2 -o "${CMAKE_CURRENT_BINARY_DIR}/dimension_padding.spv"
    DEPENDS "${IQM_ROOT}/tests/dimension_padding.comp" VERBATIM)
add_custom_target(dimension-padding-shader DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/dimension_padding.spv")
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/mixed_normalization.spv"
    COMMAND Vulkan::glslc "${IQM_ROOT}/tests/mixed_normalization.comp" --target-env=vulkan1.2 -o "${CMAKE_CURRENT_BINARY_DIR}/mixed_normalization.spv"
    DEPENDS "${IQM_ROOT}/tests/mixed_normalization.comp" VERBATIM)
add_custom_target(mixed-normalization-shader DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/mixed_normalization.spv")
add_executable(mixed-normalization-test "${IQM_ROOT}/tests/verify_mixed_normalization.cpp"
    "${IQM_ROOT}/bin/shared/vulkan_res.cpp" "${IQM_ROOT}/bin/IQM/vulkan_instance.cpp" "${IQM_ROOT}/src/base/vulkan_runtime.cpp")
add_dependencies(mixed-normalization-test mixed-normalization-shader)
target_compile_definitions(mixed-normalization-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(mixed-normalization-test PRIVATE "${IQM_ROOT}/include" "${IQM_ROOT}/bin/shared" "${IQM_ROOT}/bin/IQM")
target_link_libraries(mixed-normalization-test PRIVATE Vulkan::Vulkan)
if(MSVC)
    target_compile_options(mixed-normalization-test PRIVATE /EHsc /fp:strict)
endif()
add_executable(dimension-padding-test "${IQM_ROOT}/tests/verify_dimension_padding.cpp"
    "${IQM_ROOT}/bin/shared/vulkan_res.cpp" "${IQM_ROOT}/bin/IQM/vulkan_instance.cpp"
    "${IQM_ROOT}/src/base/vulkan_runtime.cpp")
add_dependencies(dimension-padding-test dimension-padding-shader)
target_compile_definitions(dimension-padding-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(dimension-padding-test PRIVATE "${IQM_ROOT}/include" "${IQM_ROOT}/bin/shared" "${IQM_ROOT}/bin/IQM")
target_link_libraries(dimension-padding-test PRIVATE Vulkan::Vulkan)
if(MSVC)
    target_compile_options(dimension-padding-test PRIVATE /EHsc)
endif()
if(WIN32)
    add_executable(dimension-native-plan-test "${IQM_ROOT}/tests/inspect_fftw_plan.cpp")
    if(MSVC)
        target_compile_options(dimension-native-plan-test PRIVATE /EHsc /fp:strict)
    endif()
endif()
add_executable(mean120-test "${IQM_ROOT}/tests/verify_mean120.cpp"
    "${IQM_ROOT}/bin/shared/vulkan_res.cpp" "${IQM_ROOT}/bin/IQM/vulkan_instance.cpp"
    "${IQM_ROOT}/src/base/vulkan_runtime.cpp")
target_compile_definitions(mean120-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(mean120-test PRIVATE "${IQM_ROOT}/include" "${IQM_ROOT}/bin/shared" "${IQM_ROOT}/bin/IQM")
target_link_libraries(mean120-test PRIVATE Vulkan::Vulkan)
if(MSVC)
    target_compile_options(mean120-test PRIVATE /EHsc /fp:strict)
else()
    target_compile_options(mean120-test PRIVATE -ffp-contract=off)
endif()
add_executable(residual120-test "${IQM_ROOT}/tests/verify_residual120.cpp"
    "${IQM_ROOT}/bin/shared/vulkan_res.cpp" "${IQM_ROOT}/bin/IQM/vulkan_instance.cpp"
    "${IQM_ROOT}/src/base/vulkan_runtime.cpp")
target_compile_definitions(residual120-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(residual120-test PRIVATE "${IQM_ROOT}/include" "${IQM_ROOT}/bin/shared" "${IQM_ROOT}/bin/IQM")
target_link_libraries(residual120-test PRIVATE Vulkan::Vulkan)
if(MSVC)
    target_compile_options(residual120-test PRIVATE /EHsc /fp:strict)
else()
    target_compile_options(residual120-test PRIVATE -ffp-contract=off)
endif()
add_executable(residual120-pipeline-test "${IQM_ROOT}/tests/verify_residual120_pipeline.cpp"
    "${IQM_ROOT}/bin/shared/vulkan_res.cpp" "${IQM_ROOT}/bin/IQM/vulkan_instance.cpp"
    "${IQM_ROOT}/src/base/vulkan_runtime.cpp")
target_compile_definitions(residual120-pipeline-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(residual120-pipeline-test PRIVATE "${IQM_ROOT}/include" "${IQM_ROOT}/bin/shared" "${IQM_ROOT}/bin/IQM")
target_link_libraries(residual120-pipeline-test PRIVATE Vulkan::Vulkan)
if(MSVC)
    target_compile_options(residual120-pipeline-test PRIVATE /EHsc /fp:strict)
else()
    target_compile_options(residual120-pipeline-test PRIVATE -ffp-contract=off)
endif()
add_executable(base120-test "${IQM_ROOT}/tests/verify_base120.cpp"
    "${IQM_ROOT}/bin/shared/vulkan_res.cpp" "${IQM_ROOT}/bin/IQM/vulkan_instance.cpp"
    "${IQM_ROOT}/src/base/vulkan_runtime.cpp")
target_compile_definitions(base120-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(base120-test PRIVATE "${IQM_ROOT}/include" "${IQM_ROOT}/bin/shared" "${IQM_ROOT}/bin/IQM")
target_link_libraries(base120-test PRIVATE Vulkan::Vulkan)
if(MSVC)
    target_compile_options(base120-test PRIVATE /EHsc /fp:strict)
else()
    target_compile_options(base120-test PRIVATE -ffp-contract=off)
endif()
add_executable(spyr-runtime-test "${CMAKE_CURRENT_LIST_DIR}/../verify_spyr_runtime.cpp"
 "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/pathway_spyr.cpp"
 "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared/vulkan_res.cpp"
 "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM/vulkan_instance.cpp"
 "${CMAKE_CURRENT_LIST_DIR}/../../src/base/vulkan_runtime.cpp")
target_compile_definitions(spyr-runtime-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(spyr-runtime-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../include" "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared" "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM")
target_link_libraries(spyr-runtime-test PRIVATE Vulkan::Vulkan)
if(MSVC)
 target_compile_options(spyr-runtime-test PRIVATE /EHsc /fp:strict)
else()
 target_compile_options(spyr-runtime-test PRIVATE -ffp-contract=off)
endif()

add_executable(pool-runtime-test "${CMAKE_CURRENT_LIST_DIR}/../verify_pool_runtime.cpp"
 "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/pathway_pool.cpp"
 "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared/vulkan_res.cpp"
 "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM/vulkan_instance.cpp"
 "${CMAKE_CURRENT_LIST_DIR}/../../src/base/vulkan_runtime.cpp")
target_compile_definitions(pool-runtime-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(pool-runtime-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../include" "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared" "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM")
target_link_libraries(pool-runtime-test PRIVATE Vulkan::Vulkan)
if(MSVC)
 target_compile_options(pool-runtime-test PRIVATE /EHsc /fp:strict)
else()
 target_compile_options(pool-runtime-test PRIVATE -ffp-contract=off)
endif()

add_executable(comparison-runtime-test "${CMAKE_CURRENT_LIST_DIR}/../verify_comparison_runtime.cpp"
 "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/pathway_comparison.cpp"
 "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared/vulkan_res.cpp"
 "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM/vulkan_instance.cpp"
 "${CMAKE_CURRENT_LIST_DIR}/../../src/base/vulkan_runtime.cpp")
target_compile_definitions(comparison-runtime-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(comparison-runtime-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../include" "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared" "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM")
target_link_libraries(comparison-runtime-test PRIVATE Vulkan::Vulkan)
if(MSVC)
 target_compile_options(comparison-runtime-test PRIVATE /EHsc /fp:strict)
else()
 target_compile_options(comparison-runtime-test PRIVATE -ffp-contract=off)
endif()

target_sources(comparison-runtime-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/resize.cpp")

add_executable(masking-runtime-test "${CMAKE_CURRENT_LIST_DIR}/../verify_masking_runtime.cpp"
 "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/pathway_masking.cpp"
 "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared/vulkan_res.cpp"
 "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM/vulkan_instance.cpp"
 "${CMAKE_CURRENT_LIST_DIR}/../../src/base/vulkan_runtime.cpp")
target_compile_definitions(masking-runtime-test PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_include_directories(masking-runtime-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../include" "${CMAKE_CURRENT_LIST_DIR}/../../bin/shared" "${CMAKE_CURRENT_LIST_DIR}/../../bin/IQM")
target_link_libraries(masking-runtime-test PRIVATE Vulkan::Vulkan)
if(MSVC)
 target_compile_options(masking-runtime-test PRIVATE /EHsc /fp:strict)
else()
 target_compile_options(masking-runtime-test PRIVATE -ffp-contract=off)
endif()

target_sources(masking-runtime-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/resize.cpp")

target_sources(masking-runtime-test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/map_gpu.cpp" "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/csf.cpp" "${CMAKE_CURRENT_LIST_DIR}/../../src/hdrvdp3/quality.cpp")
