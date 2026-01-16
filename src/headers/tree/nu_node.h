#pragma once
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#define LAYOUT_VERTICAL              0x01  // 00000001
#define GROW_HORIZONTAL              0x02  // 00000010
#define GROW_VERTICAL                0x04  // 00000100
#define OVERFLOW_VERTICAL_SCROLL     0x08  // 00001000
#define OVERFLOW_HORIZONTAL_SCROLL   0x10  // 00010000
#define HIDE_BACKGROUND              0x20  // 00100000
#define POSITION_ABSOLUTE            0x40  // 01000000
#define HIDDEN                       0x80  // 10000000

typedef enum NodeType
{
    WINDOW, BOX, BUTTON, INPUT, CANVAS, IMAGE, TABLE, THEAD, ROW, NAT,
} NodeType;


typedef struct Node
{
    SDL_Window* window;
    char* class;
    char* id;
    char* textContent;
    
    u64 inlineStyleFlags; // --- Tracks which styles were applied in xml ---
    u16 eventFlags; // --- Event Information
    u8 positionAbsolute; // --- Tree information ---

    // --- Styling ---
    GLuint glImageHandle;
    float x, y, width, height, preferred_width, preferred_height;
    float minWidth, maxWidth, minHeight, maxHeight;
    float gap, contentWidth, contentHeight, scrollX, scrollV;
    float left, right, top, bottom;
    u8 padTop, padBottom, padLeft, padRight;
    u8 borderTop, borderBottom, borderLeft, borderRight;
    u8 borderRadiusTl, borderRadiusTr, borderRadiusBl, borderRadiusBr;
    u8 backgroundR, backgroundG, backgroundB;
    u8 borderR, borderG, borderB;
    u8 textR, textG, textB;
    u8 fontId;
    u8 layoutFlags;
    char horizontalAlignment;
    char verticalAlignment;
    char horizontalTextAlignment;
    char verticalTextAlignment;
    bool hideBackground;
} Node;

typedef struct NodeP
{
    Node node;
    NodeType type;
    u32 handle;
    u32 index;
    u32 parentHandle;
    u32 clippingRootHandle;
    u32 firstChildIndex;
    u32 childCapacity;
    u32 childCount;
    u8 layer;
    u8 state;
} NodeP;

typedef struct NU_NodeDimensions
{
    float width, height;
} NU_NodeDimensions;

typedef struct NU_ClipBounds 
{
    float clip_top;
    float clip_bottom;
    float clip_left;
    float clip_right;
    float tl_radius_x, tl_radius_y;
    float tr_radius_x, tr_radius_y;
    float bl_radius_x, bl_radius_y;
    float br_radius_x, br_radius_y;
} NU_ClipBounds;

void NU_ApplyNodeDefaults(NodeP* node)
{
    node->node.window = NULL;
    node->node.class = NULL;
    node->node.id = NULL;
    node->node.textContent = NULL;
    node->node.inlineStyleFlags = 0;
    node->node.positionAbsolute = 0;
    node->node.glImageHandle = 0;
    node->node.x = 0.0f;
    node->node.y = 0.0f;
    node->node.preferred_width = 0.0f;
    node->node.preferred_height = 0.0f;
    node->node.minWidth = 0.0f;
    node->node.maxWidth = 10e20f;
    node->node.minHeight = 0.0f;
    node->node.maxHeight = 10e20f;
    node->node.gap = 0.0f;
    node->node.contentWidth = node->node.contentHeight = 0.0f;
    node->node.scrollX = node->node.scrollV = 0.0f;
    node->node.left = node->node.right = node->node.top = node->node.bottom = -1.0f;
    node->node.padLeft = node->node.padRight = node->node.padTop = node->node.padBottom = 0;
    node->node.borderLeft = node->node.borderRight = 0;
    node->node.borderTop = node->node.borderBottom = 0;
    node->node.borderRadiusTl = node->node.borderRadiusTr = 0;
    node->node.borderRadiusBl = node->node.borderRadiusBr = 0;
    node->node.backgroundR = node->node.backgroundG = node->node.backgroundB = 50;
    node->node.borderR = node->node.borderG = node->node.borderB = 100;
    node->node.textR = node->node.textG = node->node.textB = 255;
    node->node.fontId = 0;
    node->node.layoutFlags = 0;
    node->node.horizontalAlignment = 0;
    node->node.verticalAlignment = 0;
    node->node.horizontalTextAlignment = 1;
    node->node.verticalTextAlignment = 1;
    node->node.hideBackground = 0;
    node->node.eventFlags = 0;
}
