add_library(IQM-LPIPS STATIC lpips.cpp)
add_library(IQM::LPIPS ALIAS IQM-LPIPS)

find_package(Vulkan REQUIRED)

target_include_directories(IQM-LPIPS
        PRIVATE "../../include"
        PRIVATE "../../shaders_out"
)

if(MSVC)
    target_compile_options(IQM-LPIPS PRIVATE /W4)
else()
    target_compile_options(IQM-LPIPS PRIVATE -Wall -Wextra)
endif()
target_link_libraries(IQM-LPIPS PUBLIC Vulkan::Vulkan IQM::LibBase)