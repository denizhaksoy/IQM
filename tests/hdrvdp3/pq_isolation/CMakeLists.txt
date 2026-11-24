cmake_minimum_required(VERSION 3.29)
project(HDRVDP3JndIsolation LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
add_compile_definitions(VULKAN_HPP_NO_CONSTRUCTORS)
get_filename_component(IQM_ROOT "../../.." ABSOLUTE)
add_subdirectory("${IQM_ROOT}/src/base" "${CMAKE_CURRENT_BINARY_DIR}/base" EXCLUDE_FROM_ALL)
add_subdirectory("${IQM_ROOT}/src/hdrvdp3" "${CMAKE_CURRENT_BINARY_DIR}/hdrvdp3" EXCLUDE_FROM_ALL)
add_executable(verify-pq-isolation
    verify_pq_isolation.cpp
    "${IQM_ROOT}/bin/IQM/vulkan_instance.cpp"
    "${IQM_ROOT}/bin/shared/vulkan_res.cpp")
target_include_directories(verify-pq-isolation PRIVATE "${IQM_ROOT}/include" "${IQM_ROOT}/bin/shared" "${IQM_ROOT}/bin/IQM")
target_link_directories(verify-pq-isolation PRIVATE
  "${IQM_ROOT}/build-phaseA1/lib/VkFFT/glslang-main/glslang"
  "${IQM_ROOT}/build-phaseA1/lib/VkFFT/glslang-main/glslang/OSDependent/Windows"
  "${IQM_ROOT}/build-phaseA1/lib/VkFFT/glslang-main/SPIRV")
target_link_libraries(verify-pq-isolation PRIVATE IQM::HDRVDP3 glslang MachineIndependent GenericCodeGen OSDependent SPIRV glslang-default-resource-limits)
target_compile_options(verify-pq-isolation PRIVATE /fp:strict /W4)
