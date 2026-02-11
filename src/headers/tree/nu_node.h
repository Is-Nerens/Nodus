#pragma once
#include <stdint.h>
#include "nu_input_text_struct.h"
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// Layout flags
#define LAYOUT_VERTICAL                 (1ULL << 0)
#define GROW_HORIZONTAL                 (1ULL << 1)
#define GROW_VERTICAL                   (1ULL << 2)
#define OVERFLOW_VERTICAL_SCROLL        (1ULL << 3)
#define OVERFLOW_HORIZONTAL_SCROLL      (1ULL << 4)
#define HIDE_BACKGROUND                 (1ULL << 5)
#define POSITION_ABSOLUTE               (1ULL << 6)
#define HIDDEN                          (1ULL << 7)
#define IGNORE_MOUSE                    (1ULL << 8)

// Property flags
#define PROPERTY_FLAG_LAYOUT_VERTICAL   (1ULL << 0)
#define PROPERTY_FLAG_GROW              (1ULL << 1)
#define PROPERTY_FLAG_VERTICAL_SCROLL   (1ULL << 2)
#define PROPERTY_FLAG_HORIZONTAL_SCROLL (1ULL << 3)
#define PROPERTY_FLAG_POSITION_ABSOLUTE (1ULL << 4)
#define PROPERTY_FLAG_HIDDEN            (1ULL << 5)
#define PROPERTY_FLAG_IGNORE_MOUSE      (1ULL << 6)
#define PROPERTY_FLAG_GAP               (1ULL << 7)
#define PROPERTY_FLAG_PREFERRED_WIDTH   (1ULL << 8)
#define PROPERTY_FLAG_MIN_WIDTH         (1ULL << 9)
#define PROPERTY_FLAG_MAX_WIDTH         (1ULL << 10)
#define PROPERTY_FLAG_PREFERRED_HEIGHT  (1ULL << 11)
#define PROPERTY_FLAG_MIN_HEIGHT        (1ULL << 12)
#define PROPERTY_FLAG_MAX_HEIGHT        (1ULL << 13)
#define PROPERTY_FLAG_ALIGN_H           (1ULL << 14)
#define PROPERTY_FLAG_ALIGN_V           (1ULL << 15)
#define PROPERTY_FLAG_TEXT_ALIGN_H      (1ULL << 16)
#define PROPERTY_FLAG_TEXT_ALIGN_V      (1ULL << 17)
#define PROPERTY_FLAG_LEFT              (1ULL << 18)
#define PROPERTY_FLAG_RIGHT             (1ULL << 19)
#define PROPERTY_FLAG_TOP               (1ULL << 20)
#define PROPERTY_FLAG_BOTTOM            (1ULL << 21)
#define PROPERTY_FLAG_BACKGROUND        (1ULL << 22)
#define PROPERTY_FLAG_HIDE_BACKGROUND   (1ULL << 23)
#define PROPERTY_FLAG_BORDER_COLOUR     (1ULL << 24)
#define PROPERTY_FLAG_TEXT_COLOUR       (1ULL << 25)
#define PROPERTY_FLAG_BORDER_TOP        (1ULL << 26)
#define PROPERTY_FLAG_BORDER_BOTTOM     (1ULL << 27)
#define PROPERTY_FLAG_BORDER_LEFT       (1ULL << 28)
#define PROPERTY_FLAG_BORDER_RIGHT      (1ULL << 29)
#define PROPERTY_FLAG_BORDER_RADIUS_TL  (1ULL << 30)
#define PROPERTY_FLAG_BORDER_RADIUS_TR  (1ULL << 31)
#define PROPERTY_FLAG_BORDER_RADIUS_BL  (1ULL << 32)
#define PROPERTY_FLAG_BORDER_RADIUS_BR  (1ULL << 33)
#define PROPERTY_FLAG_PAD_TOP           (1ULL << 34)
#define PROPERTY_FLAG_PAD_BOTTOM        (1ULL << 35)
#define PROPERTY_FLAG_PAD_LEFT          (1ULL << 36)
#define PROPERTY_FLAG_PAD_RIGHT         (1ULL << 37)
#define PROPERTY_FLAG_IMAGE             (1ULL << 38)
#define PROPERTY_FLAG_INPUT_TYPE        (1ULL << 39)


#define NODEP_OF(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

typedef enum NodeType
{
    NU_WINDOW, NU_BOX, NU_BUTTON, NU_INPUT, NU_CANVAS, NU_IMAGE, NU_TABLE, NU_THEAD, NU_ROW, NU_NAT,
} NodeType;

typedef struct InputTypeData {
    InputText inputText;
} InputTypeData;

typedef struct ImageTypeData {
    GLuint glImageHandle;
} ImageTypeData;

typedef union NodeTypeData {
    InputTypeData input;
    ImageTypeData image;
} NodeTypeData;

typedef struct Node
{
    SDL_Window* window;
    char* class;
    char* id;
    char* textContent;
    
    u64 inlineStyleFlags; // --- Tracks which styles were applied in xml ---

    // --- Styling ---
    float x, y, width, height, preferred_width, preferred_height;
    float minWidth, maxWidth, minHeight, maxHeight;
    float gap, contentWidth, contentHeight, scrollX, scrollV;
    float left, right, top, bottom;
    u16 eventFlags; // --- Event Information
    u16 layoutFlags;
    u8 padTop, padBottom, padLeft, padRight;
    u8 borderTop, borderBottom, borderLeft, borderRight;
    u8 borderRadiusTl, borderRadiusTr, borderRadiusBl, borderRadiusBr;
    u8 backgroundR, backgroundG, backgroundB;
    u8 borderR, borderG, borderB;
    u8 textR, textG, textB;
    u8 fontId;
    char horizontalAlignment;
    char verticalAlignment;
    char horizontalTextAlignment;
    char verticalTextAlignment;
    u8 positionAbsolute;
} Node;

typedef struct NodeP
{
    Node node;
    NodeType type;
    NodeTypeData typeData;
    struct NodeP* parent;
    struct NodeP* nextSibling;
    struct NodeP* prevSibling;
    struct NodeP* firstChild;
    struct NodeP* lastChild;
    struct NodeP* clippedAncestor;
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
    // zero init
    memset(&node->node, 0, sizeof(node->node));
    memset(&node->typeData, 0, sizeof(node->typeData));

    node->node.window = NULL;
    node->node.class = NULL;
    node->node.id = NULL;
    node->node.textContent = NULL;
    node->node.maxWidth = 10e20f;
    node->node.maxHeight = 10e20f;
    node->node.left = node->node.right = node->node.top = node->node.bottom = -1.0f;
    node->node.backgroundR = node->node.backgroundG = node->node.backgroundB = 50;
    node->node.borderR = node->node.borderG = node->node.borderB = 100;
    node->node.textR = node->node.textG = node->node.textB = 255;
    node->node.horizontalTextAlignment = 1;
    node->node.verticalTextAlignment = 1;
    
    // set defaults based on node type
    if (node->type == NU_TABLE) {
        node->node.inlineStyleFlags |= PROPERTY_FLAG_LAYOUT_VERTICAL; // Enforce vertical direction
        node->node.layoutFlags |= LAYOUT_VERTICAL;
    }
    else if (node->type == NU_THEAD || node->type == NU_ROW) {
        node->node.inlineStyleFlags |= PROPERTY_FLAG_GROW; // Enforce horizontal growth
        node->node.layoutFlags |= GROW_HORIZONTAL;
    }
    else if (node->type == NU_INPUT) {
        InputText_Init(&node->typeData.input.inputText);
    }
    if (node->type != NU_WINDOW) {
        node->node.window = node->parent->node.window;
    }
}
