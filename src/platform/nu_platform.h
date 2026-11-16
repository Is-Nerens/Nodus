#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS
    #include "nu_macos.h"
#elif defined(__linux__) || defined(__unix__)
    #define PLATFORM_LINUX
#else
    #error "Unsupported platform"
#endif


// MacOS MoltenVK / KosmicKrisp portability
#if defined(PLATFORM_MACOS)
    #if defined(PLATFORM_MACOS_INTEL)
        #define VULKAN_BACKEND_MOLTENVK 1
        #define VULKAN_BACKEND_KOSMICKRISP 0
    #else
        #define VULKAN_BACKEND_MOLTENVK 0
        #define VULKAN_BACKEND_KOSMICKRISP 1
    #endif
#else
    #define VULKAN_BACKEND_MOLTENVK 0
    #define VULKAN_BACKEND_KOSMICKRISP 0
#endif

// Vulkan backend portability requirements
#if defined(PLATFORM_MACOS) && VULKAN_BACKEND == VULKAN_BACKEND_MOLTENVK
    #define NU_VK_REQUIRED_INSTANCE_EXTENSIONS \
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
    #define NU_VK_INSTANCE_FLAGS \
        VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
    #define NU_VK_REQUIRED_DEVICE_EXTENSIONS \
        VK_KHR_SWAPCHAIN_EXTENSION_NAME, \
        "VK_KHR_portability_subset"
#else
    #define NU_VK_REQUIRED_INSTANCE_EXTENSIONS
    #define NU_VK_INSTANCE_FLAGS 0
    #define NU_VK_REQUIRED_DEVICE_EXTENSIONS \
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
#endif
