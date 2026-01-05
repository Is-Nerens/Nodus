#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> 
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <freetype/freetype.h>

#ifdef __cplusplus
extern "C" {
#endif




// Opaque structs
typedef struct NU_Nodelist NU_Nodelist;
typedef struct NU_Stylesheet NU_Stylesheet;
typedef struct Vector
{
    uint32_t capacity;
    uint32_t size;
    void* data;
    uint32_t element_size;
} Vector;
typedef struct {
    struct Vector freelist;
    char** strings_map;        // sparse hash vector of char*
    char** buffer_chunks;      // array of char arrays
    uint32_t chunk_size;       // chars in each chunk
    uint16_t chunks_used;      // number of chunks
    uint16_t chunks_available; // number of chunks
    uint32_t NU_String_Map_capacity;
    uint32_t total_buffer_capacity;
    uint32_t string_count;
    uint32_t max_probes;
} String_Set; 
typedef struct {
    uint32_t chunk;
    uint32_t index;
    uint32_t size;
} NU_String_Map_Free_Element;
typedef struct {
    char** strings_map;        // sparse hash vector of char*
    char** buffer_chunks;      // array of char arrays (stores keys)
    void* item_data;           // sparse array of items
    NU_String_Map_Free_Element* freelist;
    uint32_t first_chunk_size; // chars in first chunk
    uint16_t chunks_used;      // number of chunks
    uint16_t chunks_available; // number of chunks
    uint32_t NU_String_Map_capacity;
    uint32_t total_buffer_capacity;
    uint32_t string_count;
    uint32_t item_size;
    uint32_t freelist_capacity;
    uint32_t freelist_size;
    uint32_t max_probes;
} NU_String_Map;
struct NU_Hashmap
{
    uint8_t* occupancy;
    void* data;
    uint32_t key_size;
    uint32_t item_size;
    uint32_t item_count;
    uint32_t capacity;
    uint32_t max_probes;
};

// Visible structs
enum Tag
{
    WINDOW,
    REC,
    BUTTON,
    GRID,
    CANVAS,
    IMAGE,
    TABLE,
    THEAD,
    ROW,
    NAT,
};

struct Node
{
    SDL_Window* window;
    char* class;
    char* id;
    char* text_content;
    uint64_t inline_style_flags;
    uint16_t event_flags;
    uint32_t handle;
    uint32_t clipping_root_handle;
    uint16_t index;
    uint16_t parent_index;
    uint16_t first_child_index;
    uint16_t child_capacity;
    uint16_t child_count;
    uint8_t node_present;
    uint8_t layer; 
    uint8_t position_absolute;
    enum Tag tag;
    GLuint gl_image_handle;
    float x, y, width, height, preferred_width, preferred_height;
    float min_width, max_width, min_height, max_height;
    float gap, content_width, content_height, scroll_x, scroll_v;
    float left, right, top, bottom;
    uint8_t pad_top, pad_bottom, pad_left, pad_right;
    uint8_t border_top, border_bottom, border_left, border_right;
    uint8_t border_radius_tl, border_radius_tr, border_radius_bl, border_radius_br;
    uint8_t background_r, background_g, background_b;
    uint8_t border_r, border_g, border_b;
    uint8_t text_r, text_g, text_b;
    uint8_t font_id;
    uint8_t layout_flags;
    char horizontal_alignment;
    char vertical_alignment;
    char horizontal_text_alignment;
    char vertical_text_alignment;
    bool hide_background;
};
typedef struct {
    float r, g, b;
} NU_RGB;
enum NU_Event_Type
{
    NU_EVENT_ON_CLICK,
    NU_EVENT_ON_CHANGED,
    NU_EVENT_ON_DRAG,
    NU_EVENT_ON_RELEASED,
    NU_EVENT_ON_RESIZE,
    NU_EVENT_ON_MOUSE_DOWN,
    NU_EVENT_ON_MOUSE_UP,
    NU_EVENT_ON_MOUSE_MOVED,
    NU_EVENT_ON_MOUSE_IN,
    NU_EVENT_ON_MOUSE_OUT,
};
typedef struct NU_Event_Info_Mouse
{
    int mouse_btn;
    int mouse_x, mouse_y;
    float delta_x, delta_y;
} NU_Event_Info_Mouse;
typedef struct NU_Event
{
    uint32_t handle;
    NU_Event_Info_Mouse mouse;
} NU_Event;
typedef void (*NU_Callback)(NU_Event event, void* args);
struct NU_Callback_Info
{
    NU_Event event;
    void* args;
    NU_Callback callback;
};



// UI functions
__declspec(dllimport) int NU_Init(void);
__declspec(dllimport) void NU_Quit(void);
__declspec(dllimport) int NU_Running(void);
__declspec(dllimport) void NU_Unblock(void);
__declspec(dllimport) int NU_Load_XML(char* filepath);

// Stylesheet functions
__declspec(dllimport) uint32_t NU_Load_Stylesheet(char* css_filepath);
__declspec(dllimport) int NU_Apply_Stylesheet(uint32_t stylesheet_handle);

// DOM functions
__declspec(dllimport) inline struct Node* NU_NODE(uint32_t handle);
__declspec(dllimport) uint32_t NU_Get_Node_By_Id(char* id);
__declspec(dllimport) NU_Nodelist NU_Get_Nodes_By_Class(char* class_name);
__declspec(dllimport) NU_Nodelist NU_Get_Nodes_By_Tag(enum Tag tag);
__declspec(dllimport) uint32_t NU_Create_Node(uint32_t parent_handle, enum Tag tag);
__declspec(dllimport) void NU_Delete_Node(uint32_t handle);
__declspec(dllimport) void NU_Set_Class(uint32_t handle, char* class_name);
__declspec(dllimport) void NU_Show(uint32_t handle);
__declspec(dllimport) void NU_Hide(uint32_t handle);

// Event functions
__declspec(dllimport) void NU_Register_Event(
  uint32_t node_handle, 
  void* args,
  NU_Callback callback, 
  enum NU_Event_Type event
); 

// Canvas functions
__declspec(dllimport) void NU_Clear_Canvas(uint32_t canvas_handle);

__declspec(dllimport) void NU_Render();

__declspec(dllimport) void NU_Border_Rect(
    uint32_t canvas_handle,
    float x, float y, float w, float h,
    float thickness,
    NU_RGB* border_col,
    NU_RGB* fill_col
);

__declspec(dllimport) void NU_Line(
    uint32_t canvas_handle,
    float x1, float y1, float x2, float y2,
    float thickness,
    NU_RGB* col
);

__declspec(dllimport) void NU_Dashed_Line(
    uint32_t canvas_handle,
    float x1, float y1, float x2, float y2,
    float thickness,
    uint8_t* dash_pattern,
    uint32_t dash_pattern_len,
    NU_RGB* col
);

#ifdef __cplusplus
}
#endif