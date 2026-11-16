#pragma once

// Library includes
#define SDL_MAIN_USE_CALLBACKS
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_main.h>
#include <ft2build.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H

#include <platform/nu_platform.h>

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
#include <utils/nu_types.h>
//#include <utils/nu_simd.h>
#include <utils/nu_time.h>
#include <utils/nu_color.h>
#include <utils/nu_type_parse.h>
#include <utils/nu_performance.h>
#include <utils/nu_threadpool.h>
#include <datastructures/array.h>
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
#include <datastructures/token_array.h>
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
#include <components/canvas/nu_canvas.h>
#include <components/text input/nu_text_input.h>
#include <templates/xml/nu_xml.h>
#include <core/nu_layout.h>
#include <core/nu_draw.h>
#include <core/events/nu_events.h>
