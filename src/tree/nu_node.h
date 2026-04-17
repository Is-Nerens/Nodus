#pragma once
#include <input_text/nu_input_text_struct.h>

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


#define NODEP_OF(ptr) ((NodeP *)((char *)(ptr) - offsetof(NodeP, node)))

typedef enum NodeType
{
    NU_WINDOW, NU_BOX, NU_BUTTON, 
    NU_INPUT, NU_CANVAS, NU_IMAGE, 
    NU_TABLE, NU_THEAD, NU_ROW, NU_NAT,
    NU_FRAME
} NodeType;

typedef struct InputTypeData {
    InputText inputText;
} InputTypeData;

typedef struct ImageTypeData {
    GLuint glImageHandle;
} ImageTypeData;

typedef struct CanvasTypeData {
    int ctxHandle;
} CanvasTypeData;


typedef union NodeTypeData {
    InputTypeData input;
    ImageTypeData image;
    CanvasTypeData canvas;
} NodeTypeData;

typedef struct Node
{
    char* textContent;
    float x, y, width, height;
    float contentWidth, contentHeight;
    u16 prefWidth, prefHeight;
    u16 minWidth, maxWidth, minHeight, maxHeight;
    i16 left, right, top, bottom;
    u8 gap, padTop, padBottom, padLeft, padRight;
    u8 borderTop, borderBottom, borderLeft, borderRight;
    u8 borderRadiusTl, borderRadiusTr, borderRadiusBl, borderRadiusBr;
    u8 backgroundR, backgroundG, backgroundB;
    u8 borderR, borderG, borderB;
    u8 textR, textG, textB;
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
    char* class;
    char* id;
    u64 overrideStyleFlags;
    float scrollX, scrollV;
    u16 childCount;
    u16 eventFlags;
    u16 layoutFlags;
    u8 layer;
    u8 state;
    u8 fontId;
    u8 windowID;
    char horizontalAlignment;
    char verticalAlignment;
    char horizontalTextAlignment;
    char verticalTextAlignment;
    u8 positionAbsolute;
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
    
    node->class = NULL;
    node->id = NULL;
    node->node.textContent = NULL;
    node->layoutFlags = 0;
    node->node.maxWidth = UINT16_MAX;
    node->node.maxHeight = UINT16_MAX;
    node->node.left = node->node.right = node->node.top = node->node.bottom = -1;
    node->node.backgroundR = node->node.backgroundG = node->node.backgroundB = 50;
    node->node.borderR = node->node.borderG = node->node.borderB = 100;
    node->node.textR = node->node.textG = node->node.textB = 255;
    node->overrideStyleFlags = 0;
    node->node.prefWidth = 0;
    node->node.prefHeight = 0;
    node->fontId = 0;
    node->horizontalAlignment = 0;
    node->verticalAlignment = 0;
    node->horizontalTextAlignment = 1;
    node->verticalTextAlignment = 1;
    node->positionAbsolute = 0;
    
    // Set defaults based on node type
    switch (node->type)
    {
    case NU_TABLE:
        node->overrideStyleFlags |= PROPERTY_FLAG_LAYOUT_VERTICAL;
        node->layoutFlags |= LAYOUT_VERTICAL;
        break;
    case NU_THEAD:
        node->overrideStyleFlags |= PROPERTY_FLAG_GROW;
        node->layoutFlags |= GROW_HORIZONTAL;
        break;
    case NU_ROW:
        node->overrideStyleFlags |= PROPERTY_FLAG_GROW;
        node->layoutFlags |= GROW_HORIZONTAL;
        break;
    case NU_INPUT:
        InputText_Init(&node->typeData.input.inputText);
        break;
    case NU_CANVAS:
        node->typeData.canvas.ctxHandle = -1;
        break;
    case NU_FRAME:
        node->overrideStyleFlags |= PROPERTY_FLAG_HIDE_BACKGROUND;
        break;
    default:
        break;
    }
    if (node->type != NU_WINDOW) {
        node->windowID = node->parent->windowID;
    }
}
