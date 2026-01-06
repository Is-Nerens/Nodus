// -----------------------------------------------------------------------------------------
// --- | PURPOSE:      a collection of structs and methods regarding individual nodes
// --- | USED IN:      nodeus.h (and used just about everywhere)
// --- | DEPENDENCIES: <SDL3/SDL.h> <GL/glew.h>
// -----------------------------------------------------------------------------------------


#pragma once

// --- LAYOUT FLAGS ---
#define LAYOUT_VERTICAL              0x01  // 00000001
#define GROW_HORIZONTAL              0x02  // 00000010
#define GROW_VERTICAL                0x04  // 00000100
#define OVERFLOW_VERTICAL_SCROLL     0x08  // 00001000
#define OVERFLOW_HORIZONTAL_SCROLL   0x10  // 00010000
#define HIDE_BACKGROUND              0x20  // 00100000
#define POSITION_ABSOLUTE            0x40  // 01000000
#define HIDDEN                       0x80  // 10000000

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
    char* textContent;

    // --- Tracks which styles were applied in xml ---
    uint64_t inlineStyleFlags;

    // --- Event Information
    uint16_t eventFlags;

    // --- Tree information ---
    uint32_t handle;
    uint32_t clippingRootHandle;
    uint16_t index;
    uint16_t parentIndex;
    uint16_t firstChildIndex;
    uint16_t childCapacity;
    uint16_t childCount;
    uint8_t nodeState;
    uint8_t layer; 
    uint8_t positionAbsolute;

    // --- Styling ---
    enum Tag tag;
    GLuint glImageHandle;
    float x, y, width, height, preferred_width, preferred_height;
    float minWidth, maxWidth, minHeight, maxHeight;
    float gap, contentWidth, contentHeight, scrollX, scrollV;
    float left, right, top, bottom;
    uint8_t padTop, padBottom, padLeft, padRight;
    uint8_t borderTop, borderBottom, borderLeft, borderRight;
    uint8_t borderRadiusTl, borderRadiusTr, borderRadiusBl, borderRadiusBr;
    uint8_t backgroundR, backgroundG, backgroundB;
    uint8_t borderR, borderG, borderB;
    uint8_t textR, textG, textB;
    uint8_t fontId;
    uint8_t layoutFlags;
    char horizontalAlignment;
    char verticalAlignment;
    char horizontalTextAlignment;
    char verticalTextAlignment;
    bool hideBackground;
};

typedef struct NU_Node_Dimensions
{
    float width, height;
} NU_Node_Dimensions;

typedef struct NU_Clip_Bounds 
{
    float clip_top;
    float clip_bottom;
    float clip_left;
    float clip_right;
    float tl_radius_x, tl_radius_y;
    float tr_radius_x, tr_radius_y;
    float bl_radius_x, bl_radius_y;
    float br_radius_x, br_radius_y;
} NU_Clip_Bounds;

static void NU_Apply_Node_Defaults(struct Node* node)
{
    node->window = NULL; node->class = NULL; node->id = NULL; node->textContent = NULL;
    node->inlineStyleFlags = 0;
    node->clippingRootHandle = 0;
    node->positionAbsolute = 0;
    node->tag = NAT;
    node->glImageHandle = 0;
    node->preferred_width = 0.0f;
    node->preferred_height = 0.0f;
    node->minWidth = 0.0f;  node->maxWidth = 10e20f;
    node->minHeight = 0.0f; node->maxHeight = 10e20f;
    node->gap = 0.0f;
    node->contentWidth = 0.0f; node->contentHeight = 0.0f;
    node->scrollX = 0.0f;
    node->scrollV = 0.0f;
    node->left = -1.0f;
    node->right = -1.0f;
    node->top = -1.0f;
    node->bottom = -1.0f;
    node->index = UINT16_MAX;
    node->parentIndex = UINT16_MAX;
    node->firstChildIndex = UINT16_MAX;
    node->childCapacity = 0;
    node->childCount = 0;
    node->nodeState = 1;
    node->padTop = 0;
    node->padBottom = 0;
    node->padLeft = 0;
    node->padRight = 0;
    node->borderTop = 0;
    node->borderBottom = 0;
    node->borderLeft = 0;
    node->borderRight = 0;
    node->borderRadiusTl = 0;
    node->borderRadiusTr = 0;
    node->borderRadiusBl = 0;
    node->borderRadiusBr = 0;
    node->backgroundR = 50;
    node->backgroundG = 50;
    node->backgroundB = 50;
    node->borderR = 100;
    node->borderG = 100;
    node->borderB = 100;
    node->textR = 255;
    node->textG = 255;
    node->textB = 255;
    node->fontId = 0;
    node->layoutFlags = 0;
    node->horizontalAlignment = 0;
    node->verticalAlignment = 0;
    node->horizontalTextAlignment = 0;
    node->verticalTextAlignment = 1;
    node->hideBackground = false;
    node->eventFlags = 0;
}
