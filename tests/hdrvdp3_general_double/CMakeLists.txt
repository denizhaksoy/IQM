cmake_minimum_required(VERSION 3.29)
project(IQMGeneralDouble LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
get_filename_component(IQM_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
add_executable(general-double-quality ../run_general_quality_double.cpp
 "${IQM_ROOT}/src/hdrvdp3/general_quality_double.cpp"
 "${IQM_ROOT}/src/hdrvdp3/mtf.cpp" "${IQM_ROOT}/src/hdrvdp3/srgb_display.cpp"
 "${IQM_ROOT}/src/hdrvdp3/spectral.cpp" "${IQM_ROOT}/src/hdrvdp3/spectral_loader.cpp"
 "${IQM_ROOT}/src/hdrvdp3/jnd.cpp" "${IQM_ROOT}/src/hdrvdp3/jnd_double.cpp"
 "${IQM_ROOT}/src/hdrvdp3/sp0_model_double.cpp"
 "${IQM_ROOT}/src/hdrvdp3/quality_double.cpp" "${IQM_ROOT}/src/hdrvdp3/resize.cpp"
 "${IQM_ROOT}/src/hdrvdp3/csf.cpp")
target_include_directories(general-double-quality PRIVATE "${IQM_ROOT}/include")
get_target_property(GENERAL_SOURCES general-double-quality SOURCES)
list(REMOVE_ITEM GENERAL_SOURCES ../run_general_quality_double.cpp)
add_executable(general-double-contract ../verify_general_quality_contract.cpp ${GENERAL_SOURCES})
target_include_directories(general-double-contract PRIVATE "${IQM_ROOT}/include")
if(MSVC)
 target_compile_options(general-double-contract PRIVATE /fp:strict /EHsc)
else()
 target_compile_options(general-double-contract PRIVATE -ffp-contract=off)
endif()
add_executable(prepare-general-double-image ../prepare_general_double_image.cpp "${IQM_ROOT}/src/hdrvdp3/rgb_image.cpp")
target_include_directories(prepare-general-double-image PRIVATE "${IQM_ROOT}/include" "${IQM_ROOT}/lib/stb")
if(MSVC)
 target_compile_options(prepare-general-double-image PRIVATE /fp:strict /EHsc)
else()
 target_compile_options(prepare-general-double-image PRIVATE -ffp-contract=off)
endif()
if(MSVC)
 target_compile_options(general-double-quality PRIVATE /fp:strict /EHsc /W4)
else()
 target_compile_options(general-double-quality PRIVATE -ffp-contract=off)
endif()
