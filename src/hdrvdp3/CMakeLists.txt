add_library(IQM-HDRVDP3 STATIC corrdn.cpp spyr.cpp)
add_library(IQM::HDRVDP3 ALIAS IQM-HDRVDP3)

find_package(Vulkan REQUIRED)

target_include_directories(IQM-HDRVDP3
        PRIVATE "../../include"
        PRIVATE "../../shaders_out"
        PRIVATE "../../bin/shared"
)

if(MSVC)
    target_compile_options(IQM-HDRVDP3 PRIVATE /W4)
else()
    target_compile_options(IQM-HDRVDP3 PRIVATE -Wall -Wextra)
endif()
target_link_libraries(IQM-HDRVDP3 PUBLIC Vulkan::Vulkan IQM::LibBase)