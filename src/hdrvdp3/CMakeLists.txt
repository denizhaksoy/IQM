add_library(IQM-HDRVDP3 STATIC
    corrdn.cpp
    spyr.cpp
    local_adapt.cpp
    pathway_quality.cpp pathway_masking.cpp pathway_comparison.cpp pathway_pool.cpp pathway_spyr.cpp mixed_filter.cpp
    mtf.cpp
    jnd.cpp
    jnd_single.cpp
    spectral.cpp
    spectral_loader.cpp
    luminance.cpp
    evaluation.cpp
    csf.cpp
    resize.cpp
    quality.cpp
    map_gpu.cpp
)
add_library(IQM::HDRVDP3 ALIAS IQM-HDRVDP3)

find_package(Vulkan REQUIRED)

target_include_directories(IQM-HDRVDP3
        PRIVATE "../../include"
        PRIVATE "../../shaders_out"
        PRIVATE "../../bin/shared"
        PRIVATE "../../lib/VkFFT/vkFFT"
)

if(MSVC)
    set_source_files_properties(evaluation.cpp luminance.cpp spectral.cpp spectral_loader.cpp pathway_quality.cpp pathway_masking.cpp pathway_comparison.cpp pathway_pool.cpp pathway_spyr.cpp mixed_filter.cpp jnd.cpp jnd_single.cpp csf.cpp resize.cpp quality.cpp map_gpu.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
    target_compile_options(IQM-HDRVDP3 PRIVATE /W4)
else()
    set_source_files_properties(evaluation.cpp luminance.cpp spectral.cpp spectral_loader.cpp pathway_quality.cpp pathway_masking.cpp pathway_comparison.cpp pathway_pool.cpp pathway_spyr.cpp mixed_filter.cpp jnd.cpp jnd_single.cpp csf.cpp resize.cpp quality.cpp map_gpu.cpp PROPERTIES COMPILE_OPTIONS "-ffp-contract=off")
    target_compile_options(IQM-HDRVDP3 PRIVATE -Wall -Wextra)
endif()
target_link_libraries(IQM-HDRVDP3 PUBLIC Vulkan::Vulkan IQM::LibBase)
