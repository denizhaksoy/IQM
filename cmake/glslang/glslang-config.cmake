# Fixed glslang CMake config for Vulkan SDK 1.4.363.0
# Workaround for broken paths in the official glslang config

cmake_policy(PUSH)
cmake_policy(VERSION 3.29)

# Set SPIRV-Tools config location
set(SPIRV-Tools_DIR "C:/VulkanSDK/1.4.363.0/Lib/cmake/SPIRV-Tools/cmake")
set(SPIRV-Tools-opt_DIR "C:/VulkanSDK/1.4.363.0/Lib/cmake/SPIRV-Tools-opt/cmake")

# Find SPIRV-Tools
find_package(SPIRV-Tools REQUIRED)
find_package(SPIRV-Tools-opt REQUIRED)

# Create the glslang targets with correct paths
set(VULKAN_SDK_LIB_DIR "C:/VulkanSDK/1.4.363.0/Lib")
set(VULKAN_SDK_INCLUDE_DIR "C:/VulkanSDK/1.4.363.0/Include")

# Helper to set both Release and RelWithDebInfo configurations
function(set_target_location target libname)
    set_target_properties(${target} PROPERTIES
        IMPORTED_LOCATION_RELEASE "${VULKAN_SDK_LIB_DIR}/${libname}.lib"
        IMPORTED_LOCATION_RELWITHDEBINFO "${VULKAN_SDK_LIB_DIR}/${libname}.lib"
    )
endfunction()

# glslang::SPIRV - static library that links SPIRV-Tools-opt
add_library(glslang::SPIRV STATIC IMPORTED)
set_target_properties(glslang::SPIRV PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_SDK_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "SPIRV-Tools-opt"
)

# glslang::glslang - main glslang library
add_library(glslang::glslang STATIC IMPORTED)
set_target_properties(glslang::glslang PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_SDK_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "SPIRV-Tools-opt"
)
set_target_location(glslang::glslang "glslang")

# glslang::OSDependent
add_library(glslang::OSDependent STATIC IMPORTED)
set_target_location(glslang::OSDependent "OSDependent")

# glslang::MachineIndependent
add_library(glslang::MachineIndependent STATIC IMPORTED)
set_target_properties(glslang::MachineIndependent PROPERTIES
    INTERFACE_LINK_LIBRARIES "glslang::glslang"
)
set_target_location(glslang::MachineIndependent "MachineIndependent")

# glslang::GenericCodeGen
add_library(glslang::GenericCodeGen STATIC IMPORTED)
set_target_location(glslang::GenericCodeGen "GenericCodeGen")

# glslang::glslang-default-resource-limits
add_library(glslang::glslang-default-resource-limits STATIC IMPORTED)
set_target_properties(glslang::glslang-default-resource-limits PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_SDK_INCLUDE_DIR}"
)
set_target_location(glslang::glslang-default-resource-limits "glslang-default-resource-limits")

# Mark targets as found
set(glslang_FOUND TRUE)
set(glslang_INCLUDE_DIRS "${VULKAN_SDK_INCLUDE_DIR}")
set(glslang_LIBRARIES glslang::glslang glslang::SPIRV)

cmake_policy(POP)