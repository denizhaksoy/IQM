add_library(IQM-PSNR STATIC psnr.cpp)
add_library(IQM::PSNR ALIAS IQM-PSNR)

find_package(Vulkan REQUIRED)

target_include_directories(IQM-PSNR
        PRIVATE "../../include"
        PRIVATE "../../shaders_out"
)

# target_compile_options(IQM-PSNR PRIVATE "-Wall;-Wextra")
if(MSVC)
    target_compile_options(IQM-PSNR PRIVATE /W4)
else()
    target_compile_options(IQM-PSNR PRIVATE -Wall -Wextra)
endif()
target_link_libraries(IQM-PSNR PUBLIC Vulkan::Vulkan IQM::LibBase)