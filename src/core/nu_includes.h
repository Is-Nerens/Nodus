#pragma once

// Library includes
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <GL/glew.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ft2build.h>
#include <stdio.h>
#include <stdlib.h>
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H

// Platform macros
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS
#elif defined(__linux__) || defined(__unix__)
    #define PLATFORM_LINUX
#else
    #error "Unsupported platform"
#endif

// MacOS MoltenVK portability
#ifdef PLATFORM_MACOS
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

// Math compatability macros
#ifndef min
    #define min(a, b) (a < b ? a : b)
#endif
#ifndef max
    #define max(a, b) (a > b ? a : b)
#endif

// Core definitions
typedef void (*NU_Callback)(void* args);
#define NU_EVENT_TIMER 1

// Nodus includes
#include <utils/nu_int.h>
#include <utils/nu_performance.h>
#include <utils/nu_threadpool.h>
#include <datastructures/Array.h>
#include <datastructures/string.h>
#include <datastructures/container.h>
#include <datastructures/stringset.h>
#include <datastructures/hashmap.h>
#include <datastructures/set.h>
#include <datastructures/stringmap.h>
#include <datastructures/linear_stringmap.h>
#include <datastructures/linear_stringset.h>
#include <datastructures/string_arena.h>
#include <datastructures/hashmap.h>
#include <datastructures/linalloc.h>
#include <core/nu_error.h>
#include <tree/nu_tree.h>
#include <graphics/rendering/nu_vulkan_utils.h>
#include <graphics/rendering/nu_renderer_structures.h>
#include <graphics/rendering/nu_pipeline_struct.h>
#include <graphics/text/nu_text.h>
#include <tree/nu_nodelist.h>
#include <display/nu_display_structs.h>
#include <templates/stylesheet/nu_stylesheet_structs.h>
#include <graphics/image/nu_image_structs.h>
#include <core/events/nu_event_defs.h>
#include <core/nu_timer.h>
#include <core/nu_widget.h>

// Global struct
#include <core/nu_gui.h>
struct NU_GUI GUI;

// --------------------------------------------------
// --- Includes That Require Access To Global GUI ---
// --------------------------------------------------
#include <graphics/image/nu_image.h>
#include <graphics/rendering/nu_pipeline_create.h>
#include <graphics/rendering/nu_renderer.h>
#include <display/nu_display_manager.h>
#include <templates/stylesheet/nu_stylesheet.h>
#include <components/nu_canvas.h>
#include <components/text input/nu_text_input.h>
#include <templates/xml/nu_xml.h>
#include <core/nu_layout.h>
#include <core/nu_draw.h>
#include <core/events/nu_events.h>
