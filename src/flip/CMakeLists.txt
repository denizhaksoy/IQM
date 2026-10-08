add_library(IQM-FLIP STATIC flip.cpp
        flip_color_pipeline.cpp
)

add_library(IQM::FLIP ALIAS IQM-FLIP)

find_package(Vulkan REQUIRED)

target_include_directories(IQM-FLIP
        PRIVATE "../../include"
        PRIVATE "../../shaders_out"
)

# target_compile_options(IQM-FLIP PRIVATE "-Wall;-Wextra")
if(MSVC)
    target_compile_options(IQM-FLIP PRIVATE /W4)
else()
    target_compile_options(IQM-FLIP PRIVATE -Wall -Wextra)
endif()
target_link_libraries(IQM-FLIP PUBLIC Vulkan::Vulkan IQM::LibBase)