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

// State flags
#define STATE_FLAG_HIDDEN               (1 << 0)
#define STATE_FLAG_POS_ABSOLUTE         (1 << 1)
#define STATE_FLAG_DELETED              (1 << 2)

// Property flags
#define PROPERTY_FLAG_LAYOUT_DIR        (1ULL << 0)
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
#define PROPERTY_FLAG_IMAGE_WIDTH_DIM   (1ULL << 39)
#define PROPERTY_FLAG_IMAGE_HEIGHT_DIM  (1ULL << 40)
#define PROPERTY_FLAG_IMAGE_REPEAT      (1ULL << 41)
#define PROPERTY_FLAG_IMAGE_FILTER      (1ULL << 42)
#define PROPERTY_FLAG_INPUT_TYPE        (1ULL << 43)
#define PROPERTY_FLAG_FONT              (1ULL << 44)

// Image settings
#define IMAGE_DIM_INTRINSIC      0
#define IMAGE_DIM_SPECIFIC       1
#define IMAGE_DIM_ASPECT         2
#define IMAGE_REPEAT_NONE        0
#define IMAGE_REPEAT_HORIZONTAL  1
#define IMAGE_REPEAT_VERTICAL    2
#define IMAGE_REPEAT_BOTH        3
#define IMAGE_FILTER_LINEAR      0
#define IMAGE_FILTER_NEAREST     1

// Node type
#define NU_TYPE_WINDOW 0
#define NU_TYPE_BOX    1
#define NU_TYPE_BUTTON 2
#define NU_TYPE_INPUT  3
#define NU_TYPE_CANVAS 4
#define NU_TYPE_IMAGE  5
#define NU_TYPE_TABLE  6
#define NU_TYPE_THEAD  7
#define NU_TYPE_ROW    8
#define NU_TYPE_FRAME  9
#define NU_TYPE_IMPORT 10
#define NU_TYPE_UNKNOWN 11

// Get containing Node* of public node
#define NODEP_OF(ptr)((Node*)((char*)(ptr) - offsetof(Node, public)))

/* Members common to all node types */
#define NU_NODE_FIELDS \
    float x, y, width, height; \
    float contentWidth, contentHeight; \
    float scrollX, scrollV; \
    u16 preferredWidth, preferredHeight; \
    u16 minWidth, maxWidth; \
    u16 minHeight, maxHeight; \
    i16 left, right, top, bottom; \
    u8 gap; \
    u8 padTop, padBottom, padLeft, padRight; \
    u8 borderTop, borderBottom, borderLeft, borderRight; \
    u8 borderRadiusTl, borderRadiusTr; \
    u8 borderRadiusBl, borderRadiusBr; \
    u8 backgroundR, backgroundG, backgroundB; \
    u8 borderR, borderG, borderB; \
    u8 textR, textG, textB;


/* Node type opaque definitions */
typedef struct NuNode NuNode;
typedef struct NuWindow NuWindow;
typedef struct NuBox NuBox;
typedef struct NuFrame NuFrame;
typedef struct NuButton NuButton;
typedef struct NuInput NuInput;
typedef struct NuCanvas NuCanvas;
typedef struct NuImage NuImage;
typedef struct NuTable NuTable;
typedef struct NuThead NuThead;
typedef struct NuRow NuRow;

/* Node type accessible definitions */
typedef struct NuNodeAccess {
    NU_NODE_FIELDS;
} NuNodeAccess;

typedef struct NuNodeGenericAccess {
    NU_NODE_FIELDS;
    char* textContent;
} NuNodeGenericAccess;

typedef struct NuWindowAccess {
    NU_NODE_FIELDS;
    char* textContent;
} NuWindowAccess;

typedef struct NuBoxAccess {
    NU_NODE_FIELDS;
    char* textContent;
} NuBoxAccess;

typedef struct NuFrameAccess {
    NU_NODE_FIELDS;
} NuFrameAccess;

typedef struct NuButtonAccess {
    NU_NODE_FIELDS;
    char* textContent;
} NuButtonAccess;

typedef struct NuInputAccess {
    NU_NODE_FIELDS;
} NuInputAccess;

typedef struct NuCanvasAccess {
    NU_NODE_FIELDS;
} NuCanvasAccess;

typedef struct NuImageAccess {
    NU_NODE_FIELDS;
} NuImageAccess;

typedef struct NuTableAccess {
    NU_NODE_FIELDS;
} NuTableAccess;

typedef struct NuTheadAccess {
    NU_NODE_FIELDS;
} NuTheadAccess;

typedef struct NuRowAccess {
    NU_NODE_FIELDS;
} NuRowAccess;

typedef union PublicNodeStructUnion {
    NuNodeGenericAccess generic;
    NuNodeAccess common;
    NuWindowAccess window;
    NuBoxAccess box;
    NuButtonAccess button;
    NuInputAccess input;
    NuCanvasAccess canvas;
    NuImageAccess image;
    NuTableAccess table;
    NuTheadAccess thead;
    NuRowAccess row;
    NuFrameAccess frame;
} PublicNodeStructUnion;

/* Node type-specific data */
typedef struct NuInputSpecial {
    int textInputHandle;
} NuInputSpecial;

typedef struct NuImageSpecial {
    int imageHandle;
    u8 widthDimType;
    u8 heightDimType;
    u8 repeat;
    u8 filter;
} NuImageSpecial;

typedef struct NuCanvasSpecial {
    int ctxHandle;
} NuCanvasSpecial;

typedef union NodeSpecialDataUnion {
    NuInputSpecial input;
    NuImageSpecial image;
    NuCanvasSpecial canvas;
} NodeSpecialDataUnion;

/* Private node storing public and private members */
typedef struct Node
{
    PublicNodeStructUnion public;       // Publicly readible to library user
    struct Node* parent;
    struct Node* nextSibling;      // Next sibling in linked-list style array
    struct Node* prevSibling;      // Prev sibling in linked-list style array
    struct Node* firstChild;       // First child in linked-list style child array
    struct Node* lastChild;        // Last child in linked-list style child array
    struct Node* clippedAncestor;  // Optional ancestor that has render clipping -> for calculating clipping union
    char* class;
    char* id;
    u64 overrideStyleFlags;         // Bitflags determine which style properties ignore the stylesheet
    NodeSpecialDataUnion special;   // Node type-specific data
    int widgetID;
    u32 subtreeSize;
    u16 childCount;
    u16 eventFlags;                 // Bit flags indicating bound events
    u16 layoutFlags;                // Bit flags controlling layout and rendering
    bool isPartitionRoot;
    u8 type;
    u8 layer;                       // Depth of this node in the tree [0, 1, ..n]
    u8 stateFlags;                  // State bitflags (eg. "STATE_FLAG_HIDDEN")
    u8 fontIdx;                     // Index of a font stored in an array
    u8 windowID;                    // Index of a window stored in a container
    u8 horizontalAlignment;         // Left, Center, Right [0, 1, 2]
    u8 verticalAlignment;           // Top, Center, Bottom [0, 1, 2]
    u8 horizontalTextAlignment;     // Left, Center, Right [0, 1, 2]
    u8 verticalTextAlignment;       // Top, Center, Bottom [0, 1, 2]
} Node;

/* Other structs used by the layout engine */
typedef struct NU_NodeDimensions
{
    float width, height;
} NU_NodeDimensions;

typedef struct NU_ClipBounds
{
    float top;
    float bottom;
    float left;
    float right;
} NU_ClipBounds;

typedef enum NodeOverlap
{
    NODE_OVERLAP_NONE,
    NODE_OVERLAP_PARTIAL,
    NODE_OVERLAP_INSIDE
} NodeOverlap;

/* Useful quality of life functions for querying node state */
inline bool NodeStateHidden(Node* node)
{
    return (node->stateFlags & STATE_FLAG_HIDDEN) != 0;
}

inline bool NodeStatePosAbsolute(Node* node)
{
    return (node->stateFlags & STATE_FLAG_POS_ABSOLUTE) != 0;
}

inline bool NodeStateDeleted(Node* node)
{
    return (node->stateFlags & STATE_FLAG_DELETED) != 0;
}

void ApplyNodeDefaults(Node* node, Node* parent, u8 type, u16 layer)
{
    // Zero init
    memset(node, 0, sizeof(Node));

    // Init members passed as args to this function
    node->parent = parent;
    node->layer = layer;
    node->type = type;

    // Set specific fields to default values
    node->public.generic.maxWidth = node->public.generic.maxHeight = UINT16_MAX;
    node->public.generic.backgroundR = node->public.generic.backgroundG = node->public.generic.backgroundB = 50;
    node->public.generic.borderR = node->public.generic.borderG = node->public.generic.borderB = 100;
    node->public.generic.textR = node->public.generic.textG = node->public.generic.textB = 255;
    node->horizontalTextAlignment = 0;
    node->verticalTextAlignment = 1;
    node->widgetID = -1;
    node->subtreeSize = 1;

    // Set defaults based on node type
    switch (node->type)
    {
    case NU_TYPE_TABLE:
        node->overrideStyleFlags |= PROPERTY_FLAG_LAYOUT_DIR;
        node->layoutFlags |= LAYOUT_VERTICAL;
        break;
    case NU_TYPE_THEAD:
        node->overrideStyleFlags |= PROPERTY_FLAG_GROW;
        node->layoutFlags |= GROW_HORIZONTAL;
        break;
    case NU_TYPE_ROW:
        node->overrideStyleFlags |= PROPERTY_FLAG_GROW;
        node->layoutFlags |= GROW_HORIZONTAL;
        break;
    case NU_TYPE_INPUT:
        node->special.input.textInputHandle = -1;
        break;
    case NU_TYPE_CANVAS:
        node->special.canvas.ctxHandle = -1;
        break;
    case NU_TYPE_FRAME:
        node->overrideStyleFlags |= PROPERTY_FLAG_HIDE_BACKGROUND;
        node->layoutFlags |= HIDE_BACKGROUND;
        break;
    default:
        break;
    }

    // Inherit parent's window (if node is not a window itself)
    if (node->type != NU_TYPE_WINDOW) {
        node->windowID = node->parent->windowID;
    }
}
