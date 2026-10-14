add_executable(IQM IQM/main.cpp IQM/args.cpp IQM/file_matcher.cpp IQM/vulkan_instance.cpp shared/methods.cpp shared/vulkan_res.cpp)
add_executable(IQM::IQM ALIAS IQM)

target_include_directories(IQM PUBLIC ../include)
target_include_directories(IQM SYSTEM PUBLIC ../lib/stb)

add_executable(IQM-profile IQM-profile/profile.cpp IQM-profile/args.cpp IQM-profile/vulkan_instance.cpp shared/methods.cpp shared/vulkan_res.cpp)
add_executable(IQM::profile ALIAS IQM-profile)

target_include_directories(IQM-profile PUBLIC ../include)
target_include_directories(IQM-profile SYSTEM PUBLIC ../lib/stb)

# target_compile_options(IQM PRIVATE "-Wall;-Wextra")
# target_compile_options(IQM-profile PRIVATE "-Wall;-Wextra")
if(MSVC)
    target_compile_options(IQM PRIVATE /W4)
    target_compile_options(IQM-profile PRIVATE /W4)
else()
    target_compile_options(IQM PRIVATE -Wall -Wextra)
    target_compile_options(IQM-profile PRIVATE -Wall -Wextra)
endif()

find_package(glfw3)
target_link_libraries(IQM-profile glfw)

target_link_libraries(IQM
        SPIRV-Tools
        SPIRV-Tools-opt
)
if(MSVC)
    target_link_libraries(IQM
        "C:/VulkanSDK/1.4.363.0/Lib/glslang.lib"
        "C:/VulkanSDK/1.4.363.0/Lib/SPIRV-Tools-opt.lib"
        "C:/VulkanSDK/1.4.363.0/Lib/SPIRV-Tools.lib"
        "C:/VulkanSDK/1.4.363.0/Lib/SPIRV.lib"
    )
endif()

target_link_libraries(IQM-profile
        SPIRV-Tools
        SPIRV-Tools-opt
)

if (ENABLE_RENDERDOC)
    target_compile_definitions(IQM PRIVATE -DENABLE_RENDERDOC)
    target_compile_definitions(IQM-profile PRIVATE -DENABLE_RENDERDOC)
endif ()

if (SSIM)
    target_compile_definitions(IQM PRIVATE -DCOMPILE_SSIM)
    target_compile_definitions(IQM-profile PRIVATE -DCOMPILE_SSIM)
    target_sources(IQM PRIVATE shared/wrappers/ssim.cpp)
    target_sources(IQM-profile PRIVATE shared/wrappers/ssim.cpp)
    target_link_libraries(IQM IQM::SSIM)
    target_link_libraries(IQM-profile IQM::SSIM)
endif ()

if (SVD)
    target_compile_definitions(IQM PRIVATE -DCOMPILE_SVD)
    target_compile_definitions(IQM-profile PRIVATE -DCOMPILE_SVD)
    target_sources(IQM PRIVATE shared/wrappers/svd.cpp)
    target_sources(IQM-profile PRIVATE shared/wrappers/svd.cpp)
    target_link_libraries(IQM IQM::SVD)
    target_link_libraries(IQM-profile IQM::SVD)
endif ()

if (FSIM)
    target_compile_definitions(IQM PRIVATE -DCOMPILE_FSIM)
    target_compile_definitions(IQM-profile PRIVATE -DCOMPILE_FSIM)
    target_sources(IQM PRIVATE shared/wrappers/fsim.cpp)
    target_sources(IQM-profile PRIVATE shared/wrappers/fsim.cpp)
    target_link_libraries(IQM IQM::FSIM)
    target_link_libraries(IQM-profile IQM::FSIM)
endif ()

if (FLIP)
    target_compile_definitions(IQM PRIVATE -DCOMPILE_FLIP)
    target_compile_definitions(IQM-profile PRIVATE -DCOMPILE_FLIP)
    target_sources(IQM PRIVATE shared/wrappers/flip.cpp)
    target_sources(IQM-profile PRIVATE shared/wrappers/flip.cpp)
    target_link_libraries(IQM IQM::FLIP)
    target_link_libraries(IQM-profile IQM::FLIP)
endif ()

if (PSNR)
    target_compile_definitions(IQM PRIVATE -DCOMPILE_PSNR)
    target_compile_definitions(IQM-profile PRIVATE -DCOMPILE_PSNR)
    target_sources(IQM PRIVATE shared/wrappers/psnr.cpp)
    target_sources(IQM-profile PRIVATE shared/wrappers/psnr.cpp)
    target_link_libraries(IQM IQM::PSNR)
    target_link_libraries(IQM-profile IQM::PSNR)
endif ()

if (LPIPS)
    target_compile_definitions(IQM PRIVATE -DCOMPILE_LPIPS)
    target_compile_definitions(IQM-profile PRIVATE -DCOMPILE_LPIPS)
    target_sources(IQM PRIVATE shared/wrappers/lpips.cpp)
    target_sources(IQM-profile PRIVATE shared/wrappers/lpips.cpp)
    target_link_libraries(IQM IQM::LPIPS)
    target_link_libraries(IQM-profile IQM::LPIPS)
endif ()

if (HDRVDP3)
    target_compile_definitions(IQM PRIVATE -DCOMPILE_HDRVDP3)
    target_compile_definitions(IQM-profile PRIVATE -DCOMPILE_HDRVDP3)
    target_sources(IQM PRIVATE shared/wrappers/hdrvdp3.cpp)
    target_sources(IQM-profile PRIVATE shared/wrappers/hdrvdp3.cpp)
    target_link_libraries(IQM IQM::HDRVDP3)
    target_link_libraries(IQM-profile IQM::HDRVDP3)
endif ()

add_executable(IQM-vkfft-test IQM-test/vkfft_test.cpp shared/vulkan_res.cpp IQM/vulkan_instance.cpp)
target_include_directories(IQM-vkfft-test PUBLIC ../include)
target_include_directories(IQM-vkfft-test PUBLIC ../bin/shared)
target_include_directories(IQM-vkfft-test PUBLIC ../bin/IQM)
target_include_directories(IQM-vkfft-test SYSTEM PUBLIC ../lib/stb)
target_include_directories(IQM-vkfft-test PUBLIC ../lib/VkFFT/vkFFT)
find_package(Vulkan REQUIRED)
if(MSVC)
    target_compile_options(IQM-vkfft-test PRIVATE /W4)
    target_link_libraries(IQM-vkfft-test 
        IQM::LibBase VkFFT Vulkan::Vulkan
        "C:/VulkanSDK/1.4.363.0/Lib/glslang.lib"
        "C:/VulkanSDK/1.4.363.0/Lib/SPIRV-Tools-opt.lib"
        "C:/VulkanSDK/1.4.363.0/Lib/SPIRV-Tools.lib"
        "C:/VulkanSDK/1.4.363.0/Lib/SPIRV.lib"
    )
else()
    target_compile_options(IQM-vkfft-test PRIVATE -Wall -Wextra)
    target_link_libraries(IQM-vkfft-test IQM::LibBase VkFFT Vulkan::Vulkan)
endif()

add_executable(IQM-mtf-gpu-test IQM-test/mtf_gpu_test.cpp shared/vulkan_res.cpp IQM/vulkan_instance.cpp)
target_include_directories(IQM-mtf-gpu-test PUBLIC ../include)
target_include_directories(IQM-mtf-gpu-test PUBLIC ../bin/shared)
target_include_directories(IQM-mtf-gpu-test PUBLIC ../bin/IQM)
target_include_directories(IQM-mtf-gpu-test SYSTEM PUBLIC ../lib/stb)
target_include_directories(IQM-mtf-gpu-test PUBLIC ../lib/VkFFT/vkFFT)
find_package(Vulkan REQUIRED)
if(MSVC)
    target_compile_options(IQM-mtf-gpu-test PRIVATE /W4)
    target_link_libraries(IQM-mtf-gpu-test 
        IQM::LibBase IQM::HDRVDP3 VkFFT Vulkan::Vulkan
        "C:/VulkanSDK/1.4.363.0/Lib/glslang.lib"
        "C:/VulkanSDK/1.4.363.0/Lib/SPIRV-Tools-opt.lib"
        "C:/VulkanSDK/1.4.363.0/Lib/SPIRV-Tools.lib"
        "C:/VulkanSDK/1.4.363.0/Lib/SPIRV.lib"
    )
else()
    target_compile_options(IQM-mtf-gpu-test PRIVATE -Wall -Wextra)
    target_link_libraries(IQM-mtf-gpu-test IQM::LibBase IQM::HDRVDP3 VkFFT Vulkan::Vulkan)
endif()

add_executable(IQM-la-fft-test IQM-test/local_adapt_fft_test.cpp shared/vulkan_res.cpp IQM/vulkan_instance.cpp)
target_include_directories(IQM-la-fft-test PUBLIC ../include)
target_include_directories(IQM-la-fft-test PUBLIC ../bin/shared)
target_include_directories(IQM-la-fft-test PUBLIC ../bin/IQM)
target_include_directories(IQM-la-fft-test SYSTEM PUBLIC ../lib/stb)
target_include_directories(IQM-la-fft-test PUBLIC ../lib/VkFFT/vkFFT)
target_include_directories(IQM-la-fft-test PRIVATE "${CMAKE_SOURCE_DIR}/shaders_out")
find_package(Vulkan REQUIRED)
if(MSVC)
    target_compile_options(IQM-la-fft-test PRIVATE /W4)
    target_link_libraries(IQM-la-fft-test 
        IQM::LibBase IQM::HDRVDP3 VkFFT Vulkan::Vulkan
        "C:/VulkanSDK/1.4.363.0/Lib/glslang.lib"
        "C:/VulkanSDK/1.4.363.0/Lib/SPIRV-Tools-opt.lib"
        "C:/VulkanSDK/1.4.363.0/Lib/SPIRV-Tools.lib"
        "C:/VulkanSDK/1.4.363.0/Lib/SPIRV.lib"
    )
else()
    target_compile_options(IQM-la-fft-test PRIVATE -Wall -Wextra)
    target_link_libraries(IQM-la-fft-test IQM::LibBase IQM::HDRVDP3 VkFFT Vulkan::Vulkan)
endif()

# HDR-VDP-3 FFT Local Adaptation integration test
# Tests LocalAdaptFFT::compute() with Vulkan images.

add_executable(IQM-la-fft-integration-test
    IQM-test/local_adapt_fft_integration_test.cpp
    shared/vulkan_res.cpp
    IQM/vulkan_instance.cpp
)

target_include_directories(IQM-la-fft-integration-test PUBLIC
    ../include
    ../bin/shared
    ../bin/IQM
    ../lib/VkFFT/vkFFT
)

target_include_directories(IQM-la-fft-integration-test SYSTEM PUBLIC
    ../lib/stb
)

target_include_directories(IQM-la-fft-integration-test PRIVATE
    "${CMAKE_SOURCE_DIR}/shaders_out"
)

if(MSVC)
    target_compile_options(IQM-la-fft-integration-test PRIVATE /W4)

    target_link_libraries(IQM-la-fft-integration-test
        IQM::LibBase
        IQM::HDRVDP3
        VkFFT
        Vulkan::Vulkan
        "C:/VulkanSDK/1.4.363.0/Lib/glslang.lib"
        "C:/VulkanSDK/1.4.363.0/Lib/SPIRV-Tools-opt.lib"
        "C:/VulkanSDK/1.4.363.0/Lib/SPIRV-Tools.lib"
        "C:/VulkanSDK/1.4.363.0/Lib/SPIRV.lib"
    )
else()
    target_compile_options(IQM-la-fft-integration-test PRIVATE -Wall -Wextra)

    target_link_libraries(IQM-la-fft-integration-test
        IQM::LibBase
        IQM::HDRVDP3
        VkFFT
        Vulkan::Vulkan
    )
endif()
add_executable(IQM-jnd-lut-test
    IQM-test/jnd_lut_test.cpp
)

target_include_directories(IQM-jnd-lut-test PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(IQM-jnd-lut-test PRIVATE
    IQM::HDRVDP3
)
add_executable(IQM-csf-test IQM-test/csf_test.cpp)

target_link_libraries(IQM-csf-test
    PRIVATE
    IQM::HDRVDP3
)

