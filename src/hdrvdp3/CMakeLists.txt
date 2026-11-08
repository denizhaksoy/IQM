add_library(IQM-HDRVDP3 STATIC
    corrdn.cpp
    spyr.cpp
    local_adapt.cpp
    sp0_model.cpp pathway_runtime.cpp pathway_quality.cpp pathway_masking.cpp pathway_comparison.cpp pathway_pool.cpp pathway_spyr.cpp mixed_filter.cpp
    sp0_double.cpp sp0_model_double.cpp
    mtf.cpp
    jnd.cpp
    jnd_double.cpp
    jnd_single.cpp
    spectral.cpp
    spectral_loader.cpp
    luminance.cpp
    evaluation.cpp
    csf.cpp
    resize.cpp
    quality.cpp
    quality_double.cpp
    map_gpu.cpp
    rgb_quality.cpp
    rgb_image.cpp
    srgb_quality96_runtime_double.cpp
    srgb_quality96_gpu_double.cpp
    srgb_quality96_double.cpp
    general_quality_double.cpp
    pq_single.cpp
    linear_color_single.cpp
    optical_response_single.cpp
    optical_single.cpp
    sp0_single.cpp
    general_quality_single.cpp
    general_single_pathway.cpp
    projection_single_gpu.cpp
    comparison_single_gpu.cpp
    mutual_single_gpu.cpp
    padding_single_gpu.cpp
    adaptation_input_single_gpu.cpp
    comparison_resize_gpu.cpp
    srgb_display.cpp
)
add_library(IQM::HDRVDP3 ALIAS IQM-HDRVDP3)

find_package(Vulkan REQUIRED)

target_include_directories(IQM-HDRVDP3
        PRIVATE "../../include"
        PRIVATE "../../shaders_out"
        PRIVATE "../../bin/shared"
        PRIVATE "../../bin/IQM"
        PRIVATE "../../lib/VkFFT/vkFFT"
)

if(MSVC)
    set_source_files_properties(pq_single.cpp linear_color_single.cpp optical_response_single.cpp optical_single.cpp sp0_single.cpp general_quality_single.cpp general_single_pathway.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
    set_source_files_properties(general_quality_double.cpp srgb_quality96_runtime_double.cpp srgb_quality96_gpu_double.cpp srgb_quality96_double.cpp rgb_quality.cpp rgb_image.cpp srgb_display.cpp jnd_double.cpp sp0_double.cpp sp0_model_double.cpp quality_double.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
    set_source_files_properties(evaluation.cpp luminance.cpp spectral.cpp spectral_loader.cpp sp0_model.cpp pathway_runtime.cpp pathway_quality.cpp pathway_masking.cpp pathway_comparison.cpp pathway_pool.cpp pathway_spyr.cpp mixed_filter.cpp jnd.cpp jnd_single.cpp csf.cpp resize.cpp quality.cpp map_gpu.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
    target_compile_options(IQM-HDRVDP3 PRIVATE /W4)
else()
    set_source_files_properties(pq_single.cpp linear_color_single.cpp optical_response_single.cpp optical_single.cpp sp0_single.cpp general_quality_single.cpp general_single_pathway.cpp PROPERTIES COMPILE_OPTIONS "-ffp-contract=off")
    set_source_files_properties(general_quality_double.cpp srgb_quality96_runtime_double.cpp srgb_quality96_gpu_double.cpp srgb_quality96_double.cpp rgb_quality.cpp rgb_image.cpp srgb_display.cpp jnd_double.cpp sp0_double.cpp sp0_model_double.cpp quality_double.cpp PROPERTIES COMPILE_OPTIONS "-ffp-contract=off")
    set_source_files_properties(evaluation.cpp luminance.cpp spectral.cpp spectral_loader.cpp sp0_model.cpp pathway_runtime.cpp pathway_quality.cpp pathway_masking.cpp pathway_comparison.cpp pathway_pool.cpp pathway_spyr.cpp mixed_filter.cpp jnd.cpp jnd_single.cpp csf.cpp resize.cpp quality.cpp map_gpu.cpp PROPERTIES COMPILE_OPTIONS "-ffp-contract=off")
    target_compile_options(IQM-HDRVDP3 PRIVATE -Wall -Wextra)
endif()
target_link_libraries(IQM-HDRVDP3 PUBLIC Vulkan::Vulkan IQM::LibBase)
