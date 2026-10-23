cmake_minimum_required(VERSION 3.29)
project(IQMBoundedEvaluation LANGUAGES CXX)
option(IQM_HDRVDP3_DIAGNOSTIC_128 "Build the separate128 reference-validation caller" OFF)
option(IQM_HDRVDP3_DIMENSION_DIAGNOSTIC "Build isolated SPYR dimension validation" OFF)
option(IQM_HDRVDP3_BUILD_APPLICATION "Build the HDR-VDP-3 application entry without other metric dependencies" OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Vulkan REQUIRED COMPONENTS glslc)
get_filename_component(IQM_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
get_filename_component(VULKAN_LIBRARY_DIRECTORY "${Vulkan_LIBRARY}" DIRECTORY)
# LocalAdaptFFT still contains the legacy VkFFT backend. Its headers and
# compiler libraries are needed to link, even when reference execution is used.
foreach(name IN ITEMS glslang SPIRV-Tools-opt SPIRV-Tools SPIRV)
    string(MAKE_C_IDENTIFIER "${name}" id)
    find_library(IQM_${id}_LIBRARY NAMES "${name}" HINTS "${VULKAN_LIBRARY_DIRECTORY}" REQUIRED)
    list(APPEND IQM_FFT_COMPILER_LIBRARIES "${IQM_${id}_LIBRARY}")
endforeach()
set(shader_names hdrvdp3_corrdn hdrvdp3_corrdn_native_tail
    hdrvdp3_local_adapt_logclamp hdrvdp3_local_adapt_pad
    hdrvdp3_local_adapt_gaussian_h hdrvdp3_local_adapt_gaussian_v
    hdrvdp3_local_adapt_crop_exp)
set(generated_directory "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${generated_directory}/hdrvdp3")
foreach(name IN LISTS shader_names)
    set(defines)
    set(source_name "${name}")
    if(name STREQUAL "hdrvdp3_corrdn_native_tail")
        set(source_name hdrvdp3_corrdn)
        set(defines -DIQM_CORRDN_NON_FMA)
    endif()
    set(source "${IQM_ROOT}/shaders/hdrvdp3/${source_name}.glsl")
    set(output "${generated_directory}/hdrvdp3/${name}.inc")
    add_custom_command(OUTPUT "${output}"
        COMMAND Vulkan::glslc ${defines} "${source}" -o "${output}" -mfmt=c --target-env=vulkan1.2
        DEPENDS "${source}" "${IQM_ROOT}/shaders/hdrvdp3/hdrvdp3_shared.glsl"
        VERBATIM)
    list(APPEND generated_headers "${output}")
endforeach()
set(module_sources corrdn.cpp spyr.cpp local_adapt.cpp mtf.cpp jnd.cpp
    jnd_single.cpp spectral.cpp spectral_loader.cpp luminance.cpp evaluation.cpp
    csf.cpp resize.cpp quality.cpp map_gpu.cpp)
list(TRANSFORM module_sources PREPEND "${IQM_ROOT}/src/hdrvdp3/")
add_library(iqm_bounded_evaluation STATIC ${module_sources} ${generated_headers}
    "${IQM_ROOT}/src/base/vulkan_runtime.cpp"
    "${IQM_ROOT}/bin/shared/vulkan_res.cpp")
target_compile_definitions(iqm_bounded_evaluation PUBLIC VULKAN_HPP_NO_CONSTRUCTORS PRIVATE VKFFT_BACKEND=0)
if(IQM_HDRVDP3_DIMENSION_DIAGNOSTIC)
    target_compile_definitions(iqm_bounded_evaluation PRIVATE IQM_HDRVDP3_DIMENSION_DIAGNOSTIC)
    add_executable(IQM-spyr-dimension-test "${IQM_ROOT}/tests/verify_spyr_dimensions.cpp"
        "${IQM_ROOT}/bin/IQM/vulkan_instance.cpp")
    target_include_directories(IQM-spyr-dimension-test PRIVATE "${IQM_ROOT}/bin/IQM")
    target_link_libraries(IQM-spyr-dimension-test PRIVATE iqm_bounded_evaluation)
    add_executable(IQM-filter-dimension-test "${IQM_ROOT}/tests/verify_filter_dimensions.cpp"
        "${IQM_ROOT}/bin/IQM/vulkan_instance.cpp")
    target_include_directories(IQM-filter-dimension-test PRIVATE "${IQM_ROOT}/bin/IQM")
    target_link_libraries(IQM-filter-dimension-test PRIVATE iqm_bounded_evaluation)
    add_executable(IQM-quality-rectangular-diagnostic "${IQM_ROOT}/tests/evaluate_quality_rectangular_diagnostic.cpp"
        "${IQM_ROOT}/bin/IQM/vulkan_instance.cpp")
    target_include_directories(IQM-quality-rectangular-diagnostic PRIVATE "${IQM_ROOT}/bin/IQM")
    target_link_libraries(IQM-quality-rectangular-diagnostic PRIVATE iqm_bounded_evaluation)
endif()
if(IQM_HDRVDP3_DIAGNOSTIC_128)
    add_executable(IQM-quality128-diagnostic "${IQM_ROOT}/tests/evaluate_quality128_diagnostic.cpp"
        "${IQM_ROOT}/bin/IQM/vulkan_instance.cpp")
    target_include_directories(IQM-quality128-diagnostic PRIVATE "${IQM_ROOT}/bin/IQM")
    target_link_libraries(IQM-quality128-diagnostic PRIVATE iqm_bounded_evaluation)
endif()
target_include_directories(iqm_bounded_evaluation PUBLIC "${IQM_ROOT}/include"
    PRIVATE "${IQM_ROOT}/bin/shared" "${IQM_ROOT}/lib/VkFFT/vkFFT" "${generated_directory}")
target_link_libraries(iqm_bounded_evaluation PUBLIC Vulkan::Vulkan PRIVATE ${IQM_FFT_COMPILER_LIBRARIES})
set(strict_sources evaluation.cpp luminance.cpp spectral.cpp spectral_loader.cpp
    jnd_single.cpp csf.cpp resize.cpp quality.cpp map_gpu.cpp)
list(TRANSFORM strict_sources PREPEND "${IQM_ROOT}/src/hdrvdp3/")
if(MSVC)
    target_compile_options(iqm_bounded_evaluation PRIVATE /EHsc)
    if(IQM_HDRVDP3_DIMENSION_DIAGNOSTIC)
        target_compile_options(IQM-spyr-dimension-test PRIVATE /EHsc /fp:strict)
        target_compile_options(IQM-filter-dimension-test PRIVATE /EHsc /fp:strict)
        target_compile_options(IQM-quality-rectangular-diagnostic PRIVATE /EHsc /fp:strict)
    endif()
    set_source_files_properties(${strict_sources} PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
    set_source_files_properties(${strict_sources} PROPERTIES COMPILE_OPTIONS "-ffp-contract=off")
endif()
add_executable(IQM-quality-api-test "${IQM_ROOT}/tests/evaluate_quality_api.cpp"
    "${IQM_ROOT}/bin/IQM/vulkan_instance.cpp")
target_include_directories(IQM-quality-api-test PRIVATE "${IQM_ROOT}/bin/IQM")
target_link_libraries(IQM-quality-api-test PRIVATE iqm_bounded_evaluation)
if(IQM_HDRVDP3_BUILD_APPLICATION)
    add_executable(IQM-hdrvdp3-args-test "${IQM_ROOT}/tests/verify_hdrvdp3_args.cpp" "${IQM_ROOT}/bin/IQM/args.cpp")
    target_include_directories(IQM-hdrvdp3-args-test PRIVATE "${IQM_ROOT}/bin/IQM")
    if(MSVC)
        target_compile_options(IQM-hdrvdp3-args-test PRIVATE /EHsc)
    endif()
    add_executable(IQM-hdrvdp3
        "${IQM_ROOT}/bin/IQM/main.cpp" "${IQM_ROOT}/bin/IQM/args.cpp"
        "${IQM_ROOT}/bin/IQM/file_matcher.cpp" "${IQM_ROOT}/bin/IQM/vulkan_instance.cpp"
        "${IQM_ROOT}/bin/shared/methods.cpp" "${IQM_ROOT}/bin/shared/wrappers/hdrvdp3.cpp")
    target_compile_definitions(IQM-hdrvdp3 PRIVATE COMPILE_HDRVDP3=1)
    target_include_directories(IQM-hdrvdp3 PRIVATE "${IQM_ROOT}/bin/IQM" "${IQM_ROOT}/bin/shared")
    target_include_directories(IQM-hdrvdp3 SYSTEM PRIVATE "${IQM_ROOT}/lib/stb")
    target_link_libraries(IQM-hdrvdp3 PRIVATE iqm_bounded_evaluation)
    if(MSVC)
        target_compile_options(IQM-hdrvdp3 PRIVATE /EHsc)
    endif()
endif()
if(MSVC)
    target_compile_options(IQM-quality-api-test PRIVATE /fp:strict /EHsc)
else()
    target_compile_options(IQM-quality-api-test PRIVATE -ffp-contract=off)
endif()
