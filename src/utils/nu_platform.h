#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS
#elif defined(__linux__) || defined(__unix__)
    #define PLATFORM_LINUX
#else
    #error "Unsupported platform"
#endif
