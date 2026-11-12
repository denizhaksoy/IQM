# The actual main IQM CLI, with only the frozen live-hybrid profile enabled.
if(NOT WIN32)
  message(FATAL_ERROR "The pinned FFTW DLL provider requires Windows")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Vulkan REQUIRED COMPONENTS glslc)
find_program(IQM_SPIRV_VAL spirv-val HINTS "$ENV{VULKAN_SDK}/Bin" REQUIRED)
set(hdr_sources pq_single linear_color_single optical_response_single optical_single
 sp0_single sp0_model_double general_quality_single general_single_pathway mtf jnd
 jnd_single spectral spectral_loader csf resize quality projection_single_gpu
 comparison_single_gpu mutual_single_gpu comparison_resize_gpu adaptation_input_single_gpu padding_single_gpu)
set(delivery_sources bin/IQM/main.cpp bin/IQM/args.cpp bin/IQM/vulkan_instance.cpp
 bin/IQM/hdrvdp3_live_quality.cpp bin/shared/hdrvdp3_single_cli.cpp bin/shared/methods.cpp
 bin/shared/vulkan_res.cpp src/base/vulkan_runtime.cpp)
foreach(name IN LISTS hdr_sources)
 list(APPEND delivery_sources "src/hdrvdp3/${name}.cpp")
endforeach()
add_executable(IQM ${delivery_sources})
target_compile_definitions(IQM PRIVATE VULKAN_HPP_NO_CONSTRUCTORS COMPILE_HDRVDP3=1
 IQM_HDRVDP3_DELIVERY_ONLY=1 IQM_HDRVDP3_LIVE_CLI=1 IQM_HDRVDP3_HYBRID=1
 IQM_HDRVDP3_COMPARISON=1 IQM_HDRVDP3_MUTUAL=1)
target_include_directories(IQM PRIVATE include bin/IQM bin/shared lib/stb)
target_link_libraries(IQM PRIVATE Vulkan::Vulkan)
if(MSVC)
 target_compile_options(IQM PRIVATE /fp:strict /EHsc /W4)
endif()
set_target_properties(IQM PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(shader_names projection mask mutual resize adaptation padding)
set(shader_sources hdrvdp3_rgb_projection_2d hdrvdp3_comparison_single hdrvdp3_mutual_single
 hdrvdp3_comparison_resize hdrvdp3_adaptation_input_single hdrvdp3_padding_single)
set(runtime_shaders)
foreach(i RANGE 0 5)
 list(GET shader_names ${i} name)
 list(GET shader_sources ${i} source)
 set(output "${CMAKE_BINARY_DIR}/runtime/${name}.spv")
 add_custom_command(OUTPUT "${output}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/runtime"
  COMMAND "${Vulkan_GLSLC_EXECUTABLE}" --target-env=vulkan1.2 -O0
   "${CMAKE_SOURCE_DIR}/shaders/hdrvdp3/${source}.comp" -o "${output}"
  COMMAND "${IQM_SPIRV_VAL}" --target-env vulkan1.2 "${output}"
  DEPENDS "${CMAKE_SOURCE_DIR}/shaders/hdrvdp3/${source}.comp" VERBATIM)
 list(APPEND runtime_shaders "${output}")
endforeach()
add_custom_target(hdrvdp3-delivery-shaders ALL DEPENDS ${runtime_shaders})
add_dependencies(IQM hdrvdp3-delivery-shaders)
install(TARGETS IQM RUNTIME DESTINATION .)
install(FILES ${runtime_shaders} DESTINATION runtime)
